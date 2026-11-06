#include "WebServer.hpp"

#include <cstring>
#include <string_view>

#include "esp_log.h"

static const char* TAG = "WEB";

static const char PAGE[] = R"HTML(<!doctype html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Wheely</title>
<style>
  :root{--bg:#111;--btn:#2a2a2e;--on:#3b82f6;--stop:#dc2626;--txt:#eee}
  *{box-sizing:border-box;-webkit-user-select:none;user-select:none;-webkit-touch-callout:none;touch-action:none}
  body{margin:0;min-height:100vh;background:var(--bg);color:var(--txt);font-family:system-ui,sans-serif;
       display:flex;flex-direction:column;align-items:center;justify-content:center;gap:20px}
  h1{margin:0;font-size:1.2rem;font-weight:600;letter-spacing:.1em}
  #status{font-size:.85rem;opacity:.8}
  #dot{display:inline-block;width:10px;height:10px;border-radius:50%;background:#777;margin-right:6px}
  .pad{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;width:min(88vw,360px);aspect-ratio:1}
  button{border:0;border-radius:16px;background:var(--btn);color:var(--txt);font-size:2rem;cursor:pointer}
  button.active{background:var(--on)}
  button.stop{background:var(--stop);font-size:1rem;font-weight:700}
  .hint{font-size:.75rem;opacity:.5}
</style></head><body>
<h1>WHEELY</h1>
<div id="status"><span id="dot"></span><span id="msg">connecting...</span></div>
<div class="pad">
  <span></span><button data-c="F">&#9650;</button><span></span>
  <button data-c="TL">&#9664;</button><button class="stop" data-c="BR">STOP</button><button data-c="TR">&#9654;</button>
  <span></span><button data-c="R">&#9660;</button><span></span>
</div>
<div class="hint">Hold a button to drive, release to stop. Keys: W A S D / arrows, space = stop</div>
<script>
const dot=document.getElementById('dot'), msg=document.getElementById('msg');
let ws, timer=null, current=null;
function connect(){
  ws=new WebSocket('ws://'+location.host+'/ws');
  ws.onopen=()=>{dot.style.background='#22c55e';msg.textContent='connected'};
  ws.onclose=()=>{dot.style.background='#ef4444';msg.textContent='disconnected, retrying...';setTimeout(connect,1000)};
}
function send(c){ if(ws&&ws.readyState===1) ws.send(c); }
function press(c,el){
  release(false);
  current=c; if(el) el.classList.add('active');
  send(c);
  if(c!=='BR') timer=setInterval(()=>send(c),200);
}
function release(sendStop=true){
  if(timer){clearInterval(timer);timer=null}
  document.querySelectorAll('button.active').forEach(b=>b.classList.remove('active'));
  if(current && current!=='BR' && sendStop) send('BR');
  current=null;
}
document.querySelectorAll('button').forEach(b=>{
  b.addEventListener('pointerdown',e=>{e.preventDefault();b.setPointerCapture(e.pointerId);press(b.dataset.c,b)});
  ['pointerup','pointercancel','lostpointercapture'].forEach(t=>b.addEventListener(t,()=>release()));
  b.addEventListener('contextmenu',e=>e.preventDefault());
});
const keys={w:'F',arrowup:'F',s:'R',arrowdown:'R',a:'TL',arrowleft:'TL',d:'TR',arrowright:'TR',' ':'BR'};
addEventListener('keydown',e=>{
  const c=keys[e.key.toLowerCase()]; if(!c||e.repeat) return; e.preventDefault();
  press(c,document.querySelector('[data-c="'+c+'"]'));
});
addEventListener('keyup',e=>{ if(keys[e.key.toLowerCase()]) release(); });
addEventListener('blur',()=>release());
connect();
</script></body></html>)HTML";

esp_err_t WebServer::Start() {
    if (m_server) return ESP_ERR_INVALID_STATE;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true; // drop stale sockets if a phone reconnects a lot

    esp_err_t err = httpd_start(&m_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t index{};
    index.uri = "/";
    index.method = HTTP_GET;
    index.handler = &WebServer::indexHandler;
    index.user_ctx = this;
    httpd_register_uri_handler(m_server, &index);

    httpd_uri_t ws{};
    ws.uri = "/ws";
    ws.method = HTTP_GET;
    ws.handler = &WebServer::wsHandler;
    ws.user_ctx = this;
    ws.is_websocket = true;
    httpd_register_uri_handler(m_server, &ws);

    esp_timer_create_args_t timerArgs{};
    timerArgs.callback = &WebServer::watchdogCb;
    timerArgs.arg = this;
    timerArgs.name = "web_watchdog";
    esp_timer_create(&timerArgs, &m_timer);
    esp_timer_start_periodic(m_timer, 200'000);

    ESP_LOGI(TAG, "Web server started on port 80");
    return ESP_OK;
}

void WebServer::Stop() {
    if (m_timer) {
        esp_timer_stop(m_timer);
        esp_timer_delete(m_timer);
        m_timer = nullptr;
    }
    if (m_server) {
        httpd_stop(m_server);
        m_server = nullptr;
    }
    m_motors.Brake();
}

esp_err_t WebServer::indexHandler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, PAGE, HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::wsHandler(httpd_req_t* req) {
    auto* self = static_cast<WebServer*>(req->user_ctx);

    if (req->method == HTTP_GET) { // WebSocket handshake
        ESP_LOGI(TAG, "Client connected");
        return ESP_OK;
    }

    httpd_ws_frame_t frame{};
    frame.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t err = httpd_ws_recv_frame(req, &frame, 0); // get length
    if (err != ESP_OK) return err;
    if (frame.len == 0 || frame.len > 8) return ESP_OK; // commands are 1-2 chars; ignore junk

    uint8_t buf[9]{};
    frame.payload = buf;
    err = httpd_ws_recv_frame(req, &frame, frame.len);
    if (err != ESP_OK) return err;

    if (frame.type == HTTPD_WS_TYPE_TEXT) {
        self->handleCommand(reinterpret_cast<const char*>(buf));
    }
    return ESP_OK;
}

void WebServer::handleCommand(const char* cmd) {
    const std::string_view c(cmd);
    bool moving = true;

    if (c == "F") m_motors.Forward();
    else if (c == "R")
        m_motors.Reverse();
    else if (c == "TR")
        m_motors.TurnRight();
    else if (c == "TL")
        m_motors.TurnLeft();
    else if (c == "C") {
        m_motors.Coast();
        moving = false;
    } else {
        m_motors.Brake();
        moving = false;
    } // "BR" or anything unknown: stop

    m_moving = moving;
    m_lastCmdUs = esp_timer_get_time();
}

void WebServer::watchdogCb(void* arg) {
    auto* self = static_cast<WebServer*>(arg);
    if (self->m_moving && (esp_timer_get_time() - self->m_lastCmdUs) > m_WATCHDOG_US) {
        self->m_moving = false;
        self->m_motors.Brake();
        ESP_LOGW(TAG, "No command received, braking");
    }
}
