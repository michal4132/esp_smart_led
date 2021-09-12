#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"

#include <sys/stat.h>
#include <dirent.h>

#include "libesphttpd/cgiflash.h"
#include "libesphttpd/cgiwebsocket.h"
#include "libesphttpd/httpd-freertos.h"
#include "libesphttpd/httpd.h"
#include "libesphttpd/route.h"
#include "libesphttpd/esp_httpd_vfs.h"

#include "httpd_task.h"
#include "led_task.h"

#define TAG "httpd_task"

static CgiStatus ICACHE_FLASH_ATTR upload_get_handler(HttpdConnData* connData);
static CgiStatus ICACHE_FLASH_ATTR list_get_handler(HttpdConnData* connData);
static CgiStatus ICACHE_FLASH_ATTR delete_handler(HttpdConnData* connData);
static void      ICACHE_FLASH_ATTR websocket_connect(Websock *ws);
static char                        connectionMemory[sizeof(RtosConnType) * MAX_CONNECTIONS];
static HttpdFreertosInstance       httpdFreertosInstance;

CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_FW,
};

HttpdBuiltInUrl builtInUrls[] = {
  ROUTE_CGI_ARG("/", cgiEspVfsGet, "/spiffs/index.html"),
  ROUTE_CGI("/list", list_get_handler),
  ROUTE_CGI_ARG("*", cgiEspVfsGet, BASE_PATH),
  ROUTE_CGI_ARG("/upload", cgiEspVfsUpload, "/spiffs/"),
	ROUTE_CGI("/upgrade", upload_get_handler),
  ROUTE_CGI("/upload", upload_get_handler),
  ROUTE_CGI("/delete", delete_handler),

  // WebSocket
  ROUTE_WS("/ws", websocket_connect),

	ROUTE_CGI_ARG("/upgrade", cgiUploadFirmware, &uploadParams),
	ROUTE_CGI("/reboot", cgiRebootFirmware),

  ROUTE_END(),
};

// minified upload.html
// file upload and ota upgrade
const char *uploadPage="<html><head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/> <title>Update</title><style>body{font-family: system-ui, -apple-system, Segoe UI, Roboto, Ubuntu, Cantarell, Noto Sans, sans-serif;}.content{font-size: 1.2em; margin-top: 18vh; text-align: center;}.btn-grad{background-image: linear-gradient(324deg,#ff6ec4,#7873f5); cursor: pointer; margin: 10px; padding: 15px 45px; text-align: center; transition: 0.5s; background-size: 200% auto; color: white; box-shadow: 0 4px 7px rgba(0, 0, 0, 0.4); border-radius: 10px; border: 0;}.btn-grad:hover{transform: scale(1.02);}.RP{--hue: 285; --holesize: 65%; --track-bg: hsl(233, 34%, 92%); height: 30vmin; width: 30vmin; min-width: 100px; min-height: 100px; display: grid; align-items: center; justify-items: center; place-items: center; position: relative; font-weight: 700; font-size: max(5.5vmin, 1.4rem); margin: auto;}.RP::before{content: \'\'; position: absolute; top: 0; bottom: 0; left: 0; right: 0; border-radius: 50%; z-index: -1; background: conic-gradient( hsl(var(--hue) 100% 70%), hsl(var(--hue) 100% 40%), hsl(var(--hue) 100% 70%) var(--progress, 0%), var(--track-bg) var(--progress, 0%) 100% ); -webkit-mask-image: radial-gradient( transparent var(--holesize), black calc(var(--holesize) + 0.5px) ); mask-image: radial-gradient( transparent var(--holesize), black calc(var(--holesize) + 0.5px) );}.file{outline: none; opacity: 0; width: 0.1px; height: 0.1px; position: absolute;}.file-input{margin: auto; width: 200px;}.file-input label{position: relative; height: 50px; border-radius: 25px; background: linear-gradient(40deg,#ff6ec4,#7873f5); box-shadow: 0 4px 7px rgba(0, 0, 0, 0.4); display: flex; align-items: center; justify-content: center; color: #fff; font-weight: bold; cursor: pointer; transition: transform .2s ease-out;}.file-name{position: absolute; bottom: -35px; left: 10px; font-size: 0.85rem; color: #555;}input:hover + label,input:focus + label{transform: scale(1.02);}input:focus + label{outline: 1px solid #000; outline: -webkit-focus-ring-color auto 2px;}</style></head><body><div class=\"content\"> <div class=\"file-input\"> <input type=\"file\" id=\"file\" class=\"file\"> <label for=\"file\"> Select file <p class=\"file-name\"></p></label> </div><br><br><div class=\"RP\" role=\"progressbar\" aria-valuenow=\"0\" aria-valuemin=\"0\" aria-valuemax=\"100\"></div><h3 id=\"status\"></h3> <button id=\"btn-send\" class=\"btn-grad\">UPLOAD</button></div><script>var p=window.location.pathname;function upload_path(f){if (p===\"/upload\"){return p+\"?filename=\"+encodeURIComponent(f);}else if(p===\"/upgrade\"){return p+\"?partition=ota_1\";}}const rP=document.querySelector(\".RP\"), file=document.querySelector(\"#file\");function _(e){return document.getElementById(e);}function reboot(){var t=new XMLHttpRequest(); t.open(\"GET\", \"/reboot\"); t.send();}function uploadFile(){var e=_(\"file\").files[0]; if(e){var t=new XMLHttpRequest(); t.upload.addEventListener(\"progress\", progressHandler, !1), t.addEventListener(\"load\", completeHandler, !1), t.addEventListener(\"error\", errorHandler, !1), t.addEventListener(\"abort\", abortHandler, !1), t.open(\"POST\", upload_path(e.name)), t.send(e);}}file.addEventListener(\"change\", (e)=>{setProgress(0); const [r]=e.target.files,{name: t, size: n}=r, a=`${t}- ${(n / 1e3).toFixed(2)}KB`; document.querySelector(\".file-name\").textContent=a;});const setProgress=(e)=>{const r=`${e}%`; rP.style.setProperty(\"--progress\", r), (rP.innerHTML=r), rP.setAttribute(\"aria-valuenow\", r);};function progressHandler(e){var r=Math.round((e.loaded / e.total) * 100); setProgress(r);}function completeHandler(e){var rsp=JSON.parse(e.target.responseText); if(rsp[\"success\"]==true){if(p===\"/upgrade\"){_(\"status\").innerHTML=\"Flash Success\"; reboot();}else if(p===\"/upload\"){_(\"status\").innerHTML=\"Upload Success\";}}}function errorHandler(e){_(\"status\").innerHTML=\"Upload Failed\";}function abortHandler(e){_(\"status\").innerHTML=\"Upload Aborted\";}_(\"btn-send\").addEventListener(\"click\", (e)=>{uploadFile();}),setProgress(0);_(\"btn-send\").innerHTML=p.replace(\"/\", \"\").toUpperCase();</script></body></html>";

static void ICACHE_FLASH_ATTR websocket_recv(Websock *ws, char *data, int len, int flags) {
  switch(data[0]){
    case SET_RGB:
      if(len == 4){
        set_color(data[1], data[2], data[3]);
      }
      break;
    case SET_WARM:
      if(len == 2){
        set_warm(data[1]);
      }
      break;
    case SET_COLD:
      if(len == 2){
        set_cold(data[1]);
      }
      break;
    case GET_STATUS:
      {
        uint8_t c_data[5];
        Color color = get_colors();
        c_data[0] = color.red;
        c_data[1] = color.green;
        c_data[2] = color.blue;
        c_data[3] = color.warm;
        c_data[4] = color.cold;
        cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, (const char*)&c_data, 5, flags);
      }
      break;
    case FADE_RGB:
      if(len == 6){
        // fadeToColor(data[1], data[2], data[3], (data[4] << 8) + data[5]);
      }
      break;
    case SET_ALL:
      if(len == 6){
        set_color(data[1], data[2], data[3]);
        set_warm(data[4]);
        set_cold(data[5]);
      }
      break;
  }
}

static void ICACHE_FLASH_ATTR websocket_connect(Websock *ws) {
	ws->recvCb=websocket_recv;
}

static CgiStatus ICACHE_FLASH_ATTR delete_handler(HttpdConnData *connData) {
  if (connData->isConnectionClosed){
    // Connection aborted. Clean up.
    return HTTPD_CGI_DONE;
  }

  if (connData->requestType!=HTTPD_METHOD_DELETE) {
		httpdStartResponse(connData, 406);
		httpdEndHeaders(connData);
		return HTTPD_CGI_DONE;
	}

  struct stat file_stat;
  char path[128];
  uint16_t len = httpdFindArg(connData->getArgs, "path", path, sizeof(path));

  if(strcmp(path, "/") == 0 || path[strlen(path) - 1] == '/' || len == 0) {
    httpdStartResponse(connData, 400);
    httpdHeader(connData, "Content-Type", "text/plain");
    httpdEndHeaders(connData);
    httpdSend(connData, "Bad path", strlen("Bad path"));
    return HTTPD_CGI_DONE;
  }

  char filepath[256];
  strcpy(filepath, BASE_PATH);
  strcat(filepath, path);

  if (stat(filepath, &file_stat) == -1) {
    httpdStartResponse(connData, 400);
    httpdHeader(connData, "Content-Type", "text/plain");
    httpdEndHeaders(connData);
    httpdSend(connData, "No file", strlen("No file"));
    return HTTPD_CGI_DONE;
  }


  unlink(filepath);

  httpdStartResponse(connData, 200);
  httpdHeader(connData, "Content-Type", "text/plain");
  httpdEndHeaders(connData);
  httpdSend(connData, "OK", strlen("OK"));
  return HTTPD_CGI_DONE;
}

static CgiStatus ICACHE_FLASH_ATTR upload_get_handler(HttpdConnData *connData) {
	LongStringState *state=connData->cgiData;
	int len;

	if (connData->isConnectionClosed) {
		//Connection aborted. Clean up.
		if (state!=NULL) free(state);
		return HTTPD_CGI_DONE;
	}

  if (connData->requestType!=HTTPD_METHOD_GET) {
		return HTTPD_CGI_NOTFOUND;
	}

	if (state==NULL) {
		//This is the first call to the CGI for this webbrowser request.
		//Allocate a state structure.
		state=malloc(sizeof(LongStringState));
		//Save the ptr in connData so we get it passed the next time as well.
		connData->cgiData=state;
		//Set initial pointer to start of string
		state->stringPos=0;
		//We need to send the headers before sending any data. Do that now.
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", "text/html");
		httpdEndHeaders(connData);
	}

	//Figure out length of string to send. We will never send more than 128 bytes in this example.
	len=strlen(uploadPage+state->stringPos); //Get remaining length
	if (len>128) len=128; //Never send more than 128 bytes

	//Send that amount of data
	httpdSend(connData, uploadPage+state->stringPos, len);
	//Adjust stringPos to first byte we haven't sent yet
	state->stringPos+=len;
	//See if we need to send more
	if (strlen(uploadPage+state->stringPos)!=0) {
		//we have more to send; let the webserver call this function again.
		return HTTPD_CGI_MORE;
	} else {
		//We're done. Clean up here as well: if the CGI function returns HTTPD_CGI_DONE, it will
		//not be called again.
		free(state);
		return HTTPD_CGI_DONE;
	}
}

static CgiStatus ICACHE_FLASH_ATTR list_get_handler(HttpdConnData *connData) {
  if (connData->isConnectionClosed){
    // Connection aborted. Clean up.
    return HTTPD_CGI_DONE;
  }

  char buff[128];
  httpdFindArg(connData->post.buff, "dir", buff, sizeof(buff));

  char entrypath[FILE_PATH_MAX];
  char entrysize[16];
  const char *entrytype;

  struct dirent *entry;
  struct stat entry_stat;

  char dirpath[FILE_PATH_MAX] = BASE_PATH;
  strcat(dirpath, "/");

  DIR *dir = opendir(dirpath);
  const size_t dirpath_len = strlen(dirpath);

  /* Retrieve the base path of file storage to construct the full path */
  strlcpy(entrypath, dirpath, sizeof(entrypath));

  if (!dir) {
    ESP_LOGE(TAG, "Failed to stat dir : %s", dirpath);
    /* Respond with 404 Not Found */
    httpdStartResponse(connData, 400);
    httpdHeader(connData, "Content-Type", "text/html");
    httpdEndHeaders(connData);
    httpdSend(connData, "Error", strlen("Error"));
    return HTTPD_CGI_DONE;
  }

  httpdStartResponse(connData, 200);
  //  httpdHeader(connData, "Content-Type", "application/json");
  httpdHeader(connData, "Content-Type", "text/html");
  httpdEndHeaders(connData);

  bool first = true;
  httpdSend(connData, "[", strlen("["));

  /* Iterate over all files / folders and fetch their names and sizes */
  while ((entry = readdir(dir)) != NULL) {
    if(first){
      first = false;
    }else{
      httpdSend(connData, ",", strlen(","));
    }
    entrytype = (entry->d_type == DT_DIR ? "dir" : "file");

   httpdSend(connData, "{\"type\":\"", strlen("{\"type\":\""));
   httpdSend(connData, entrytype, strlen(entrytype));
   httpdSend(connData, "\",\"name\":\"", strlen("\",\"name\":\""));

    strlcpy(entrypath + dirpath_len, entry->d_name, sizeof(entrypath) - dirpath_len);
    if (stat(entrypath, &entry_stat) == -1) {
      ESP_LOGE(TAG, "Failed to stat %s : %s", entrytype, entry->d_name);
      continue;
    }
    sprintf(entrysize, "%ld", entry_stat.st_size);
    ESP_LOGI(TAG, "Found %s : %s (%s bytes)", entrytype, entry->d_name, entrysize);

    /* Send chunk of HTML file containing table entries with file name and size */
    httpdSend(connData, entry->d_name, strlen(entry->d_name));
    httpdSend(connData, "\"}", strlen("\"}"));
  }
  closedir(dir);

  httpdSend(connData, "]", strlen("]"));

  return HTTPD_CGI_DONE;
}

void httpd_task_init(){
  httpdFreertosInit(&httpdFreertosInstance,
                    builtInUrls,
                    LISTEN_PORT,
                    connectionMemory,
                    MAX_CONNECTIONS,
                    HTTPD_FLAG_NONE);
  httpdFreertosStart(&httpdFreertosInstance);

  cgiEspVfsBasePath(BASE_PATH);
}
