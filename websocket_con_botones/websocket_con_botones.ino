#include <ESP8266WiFi.h> // este es para el esp8266, para el esp32 es "#include <WiFi.h>"
#include <Ticker.h>
#include <ESPAsyncTCP.h> // este es para el esp8266, para el esp32 es "#include <AsyncTCP.h>"
#include <ESPAsyncWebServer.h>
#include <sstream>
#include <string.h> // todas estas librerias son necesaria para usar los websockets, exepto "#include <Ticker.h>"

#define punto_acceso // si quiere usar el esp como un punto de acceso descomenta esta definicion, si quere que el esp se conecte a una red la comenta

AsyncWebServer server(80);
AsyncWebSocket wscomunicacion("/comunicacion");// abre el socket que se usara

std::vector<AsyncWebSocketClient *> clients; // Lista de clientes conectados

Ticker tiempo_transcurrido;// la libreria ticker se usar para llamar una funcion cada x tiempo, ejemplo cuando quieres enviar el dato de un sensor cada segundo
                           // para que se vea "en tiempo real" sin necesidad de darle a un boton para que se actualice la informacion

const char *ssid = "Conectando...";
const char *password = "casa2023";

// Configuración de IP estática
IPAddress local_IP(192, 168, 100, 100); // esta es la ip a la que debes conectarte, es una ip fija, no cambia
IPAddress gateway(192, 168, 100, 1);    // La puerta de enlace de tu red
IPAddress subnet(255, 255, 255, 0);   // Máscara de subred de tu red, la puerta de enlace y la mascara de sub red puedes sacarla escribiendo "ipconfig" en el cmd de windows

const char *htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!doctypehtml><html lang=en><meta charset=UTF-8><meta content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no"name=viewport><style>.toggle-button{position:relative;display:inline-block;padding:20px 40px;color:#fff;border:none;cursor:pointer;outline:0;overflow:hidden;border-radius:100px;background-color:#000;transition:background-color .3s ease}.toggle-checkbox{display:none}.toggle-button::before{content:'';position:absolute;top:50%;left:4%;transform:translate(0,-50%);width:40%;height:80%;background-color:#fff;border-radius:50%;transition:left .3s ease}.toggle-checkbox:checked+.toggle-button{background-color:#ff2828}.toggle-checkbox:checked+.toggle-button::before{left:calc(100% - 40% - 4%)}input[type=range]{font-size:2.8rem;width:12.5em;--track-height:0.125em;--thumb-height:1.0em;--brightness-hover:180%;--brightness-down:80%;--clip-edges:0.125em;position:relative;background:#fff0;overflow:hidden}@media (prefers-color-scheme:dark){input[type=range]{color:#ff2828;--track-color:rgba(255, 255, 255, 0.1)}}input[type=range]:active{cursor:grabbing}input[type=range]:disabled{filter:grayscale(1);opacity:.3;cursor:not-allowed}input[type=range],input[type=range]::-webkit-slider-runnable-track,input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;transition:all ease .1s;height:var(--thumb-height)}input[type=range]::-webkit-slider-runnable-track,input[type=range]::-webkit-slider-thumb{position:relative}input[type=range]::-webkit-slider-thumb{--thumb-radius:calc((var(--thumb-height) * 0.5) - 1px);--clip-top:calc((var(--thumb-height) - var(--track-height)) * 0.5 - 0.5px);--clip-bottom:calc(var(--thumb-height) - var(--clip-top));--clip-further:calc(100% + 1px);--box-fill:calc(-100vmax - var(--thumb-width, var(--thumb-height))) 0 0 100vmax currentColor;width:var(--thumb-width,var(--thumb-height));background:linear-gradient(currentColor 0 0) scroll no-repeat left center/50% calc(var(--track-height) + 1px);background-color:currentColor;box-shadow:var(--box-fill);border-radius:var(--thumb-width,var(--thumb-height));filter:brightness(100%);clip-path:polygon(100% -1px,var(--clip-edges) -1px,0 var(--clip-top),-100vmax var(--clip-top),-100vmax var(--clip-bottom),0 var(--clip-bottom),var(--clip-edges) 100%,var(--clip-further) var(--clip-further))}input[type=range]:hover::-webkit-slider-thumb{filter:brightness(var(--brightness-hover));cursor:grab}input[type=range]:active::-webkit-slider-thumb{filter:brightness(var(--brightness-down));cursor:grabbing}input[type=range]::-webkit-slider-runnable-track{background:linear-gradient(var(--track-color) 0 0) scroll no-repeat center/100% calc(var(--track-height) + 1px)}input[type=range]:disabled::-webkit-slider-thumb{cursor:not-allowed}.texto{margin:auto;text-align:center;background-color:gray;font-size:30px}td{border:2px solid}</style><body style=background-color:#292929><table style=position:absolute;top:50%;left:50%;transform:translate(-50%,-50%)><tr><td><div class=texto>Tiempo (s)</div><div style=background-color:#fff;width:70px;text-align:center;margin:auto;font-size:30px id=tiempo>0</div><div style=height:5px></div><tr><td style=text-align:center><div class=texto>Led 1</div><input id=btn_switch1 type=checkbox class=toggle-checkbox onclick=boton(1)> <label class=toggle-button for=btn_switch1></label><br><input id=slider1 type=range disabled max=1023 min=0 oninput=pwm(1) step=31 style=width:180px value=0><tr><td style=text-align:center><div class=texto>Led 2</div><input id=btn_switch2 type=checkbox class=toggle-checkbox onclick=boton(2)> <label class=toggle-button for=btn_switch2></label><br><input id=slider2 type=range disabled max=1023 min=0 oninput=pwm(2) step=31 style=width:180px value=0></table><script>function comunicacion(){websocket_comunicacion=new WebSocket("ws://"+window.location.hostname+"/comunicacion"),websocket_comunicacion.onopen=function(n){},websocket_comunicacion.onclose=function(n){setTimeout(comunicacion,2e3)},websocket_comunicacion.onmessage=function(n){document.getElementById("tiempo").innerText=n.data}}function iniciar_websockets(){comunicacion()}function enviar_datos(n,e){websocket_comunicacion.send(n+","+e)}function boton(n){document.getElementById("btn_switch"+n).checked?document.getElementById("slider"+n).disabled=!1:document.getElementById("slider"+n).disabled=!0}function pwm(n){enviar_datos("pwm"+n,document.getElementById("slider"+n).value)}window.onload=iniciar_websockets</script>
)HTMLHOMEPAGE";

void handleRoot(AsyncWebServerRequest *request)// esta funcion envia la pagina web cuando se conecta una persona al esp
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "File Not Found");
}

void tiempo_transcurridos() //esta funcion se llama automaticamente cada 1 segundo y se le envia informacion al usuario,
{                           //no llamar esta funcion igual que la variable ticker
  for (AsyncWebSocketClient *cliente : clients)// dentro de este for se le envia a cada uno de los clientes conectador "String(millis()/1000)" que en este caso es el tiempo transcurrido quede que se encendio el esp, pero en lugar de eso se pondria lo que quieres enviarle a los clientes
  {
    if (cliente->status() == WS_CONNECTED)
    {
      cliente->text(String(millis()/1000));
    }
  }
}

void datos_recibidos(const char* variable,int valor){
  if(strcmp(variable, "pwm1")==0){
    analogWrite(D4,valor);
    return;
  }
  analogWrite(D5,valor);
}

void funcion_comunicacion(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    clients.push_back(client);// cuando se conecta un cliente se agrega a la lista de clientes
    break;
  case WS_EVT_DISCONNECT:
    clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());// cuando se desconecta un cliente se elimina de la lista de clientes
    break;
  case WS_EVT_DATA: // se ingresa a este caso cuando se recive informacion desde la pagina
     AwsFrameInfo *info;
     info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
      std::string datos_entrantes = ""; // si quiere guardar los datos entrantes como un string normal usa: datos_entrantes.c_str(), esto entrega un puntero a un char (const char*) <-- esto en caso de querer guardar todo lo que se recibe junto, mas adelante se separan las variables
      datos_entrantes.assign((char *)data, len);
      std::istringstream ss(datos_entrantes);
      std::string variable, valor; // el nombre de la variable recibida se guarda en la variable "variable"
      std::getline(ss, variable, ',');
      std::getline(ss, valor, ','); // si quiere guardar "valor" como un entero usa: valor_entero = atoi(valor.c_str()); porque aqui "valor" es una variable de tipo std::string, igual que variable
      
      datos_recibidos(variable.c_str(),atoi(valor.c_str()));
      
    }
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  default:
    break;
  }
  }

void setup() {
  pinMode(D4,OUTPUT);
  pinMode(D5,OUTPUT);

  #ifdef punto_acceso
    WiFi.softAPConfig(local_IP, gateway, subnet); // Configurar IP estática para el punto de acceso
    WiFi.softAP(ssid, password); // Iniciar el punto de acceso
  #else
    WiFi.config(local_IP, gateway, subnet);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  #endif

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  wscomunicacion.onEvent(funcion_comunicacion);
  server.addHandler(&wscomunicacion);
  server.begin();

  tiempo_transcurrido.attach(1, tiempo_transcurridos); // aqui se inicia la funcion que se llama cada 1 segundo con "ticker"
}

void loop() {

}
