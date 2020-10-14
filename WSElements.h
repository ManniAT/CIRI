#include <WebServer.h>

WebServer server(80);

/*
 * Login page
 */

char strTemp[12]="??";
char strCO2[12]="??";
char strMBar[12]="??";
String strHP=
"<html><head>"
"	<meta charset='utf-8' />"
"	<title>EE895 Home</title>"
"	<style>"
"			table, th, td { border: 1px solid black; border-collapse: collapse;	}"
"			th, td { padding: 5px;	}"
"			span.valClass{font-weight:bold;}"
"	</style>"
"</head>"
"<body>"
"	<table style='margin-left: 6px'>"
"		<tr>"
"			<td style='text-align: right'><span>Temperature:</span></td>"
"			<td style='padding-right: 3px; margin-left: 6px; text-align: right'><span class='valClass'>$$T$$</span></td>"
"			<td><span>°C</span></td>"
"		</tr>"
"		<tr>"
"			<td style='text-align: right'><span>CO2:</span></td>"
"			<td style='padding-right: 3px; text-align: right'><span class='valClass'>$$C$$</span></td>"
"			<td><span>ppm</span></td>"
"		</tr>"
"		<tr>"
"			<td style='text-align: right'><span>Pressure:</span></td>"
"			<td style='padding-right: 3px; margin-left: 6px; text-align: right'><span class='valClass'>$$M$$</span></td>"
"			<td style='margin-left: 33px;'><span>mbar</span></td>"
"		</tr>"
"</table>"
"	<p><a href='ota'>Update Software</a></p>"
"</body></html>";
/*
 * OAT page
 */

const char *otaPage =
	"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
	"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
	"<input type='file' name='update'>"
	"<input type='submit' value='Update'>"
	"</form>"
	"<div id='prg'>progress: 0%</div>"
	"<p><a href='/'>Home</a></p>"
	"<script>"
	"$('form').submit(function(e){"
	"e.preventDefault();"
	"var form = $('#upload_form')[0];"
	"var data = new FormData(form);"
	" $.ajax({"
	"url: '/update',"
	"type: 'POST',"
	"data: data,"
	"contentType: false,"
	"processData:false,"
	"xhr: function() {"
	"var xhr = new window.XMLHttpRequest();"
	"xhr.upload.addEventListener('progress', function(evt) {"
	"if (evt.lengthComputable) {"
	"var per = evt.loaded / evt.total;"
	"$('#prg').html('progress: ' + Math.round(per*100) + '%');"
	"}"
	"}, false);"
	"return xhr;"
	"},"
	"success:function(d, s) {"
	"console.log('success!')"
	"},"
	"error: function (a, b, c) {"
	"}"
	"});"
	"});"
	"</script>";
String strHPResult;
void handleRoot()
{
	strHPResult = strHP;
	strHPResult.replace("$$T$$", strTemp);
	strHPResult.replace("$$C$$", strCO2);
	strHPResult.replace("$$M$$", strMBar);
	server.sendHeader("Connection", "close");
	server.send(200, "text/html", strHPResult);
}
void handleOTA()
{
	server.sendHeader("Connection", "close");
	server.send(200, "text/html", otaPage);
}
void handleUpload1()
{
	server.sendHeader("Connection", "close");
	server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
	
	ESP.restart();
}
void handleUpload2()
{
	HTTPUpload &upload = server.upload();
	if (upload.status == UPLOAD_FILE_START)
	{
		Serial.printf("Update: %s\n", upload.filename.c_str());
		if (!Update.begin(UPDATE_SIZE_UNKNOWN))
		{ //start with max available size
			Update.printError(Serial);
		}
	}
	else if (upload.status == UPLOAD_FILE_WRITE)
	{
		/* flashing firmware to ESP*/
		if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
		{
			Update.printError(Serial);
		}
	}
	else if (upload.status == UPLOAD_FILE_END)
	{
		if (Update.end(true))
		{ //true to set the size to the current progress
			Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
		}
		else
		{
			Update.printError(Serial);
		}
	}
}

void SetHPValues(float pTemp, float pMBar, ushort pCO2) {
	sprintf(strTemp, "%0.1f", pTemp);
	sprintf(strCO2, "%0d", pCO2);
	sprintf(strMBar, "%0.1f", pMBar);
}

void InitOTAServer(){
	/*return index page which is stored in serverIndex */
	server.on("/", HTTP_GET, handleRoot);
	server.on("/ota", HTTP_GET, handleOTA);
	/*handling uploading firmware file */
	server.on("/update", HTTP_POST, handleUpload1, handleUpload2);
	server.begin();	
}
//void StartWebServer(){
//	server.begin();
//}