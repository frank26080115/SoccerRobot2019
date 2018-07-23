/** Is this an IP? */
boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip)
{
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".svg")) return "image/svg+xml";
  else if (filename.endsWith(".json")) return "application/json";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) // send the right file to the client (if it exists)
{
  BookWorm.debugf("call handleFileRead: %s\r\n", path.c_str());
  //if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    serverClientStop();
    file.close();                                          // Close the file again
    #ifdef SKETCH_DEBUG
    BookWorm.printf("\tSent file: %s\r\n", path.c_str());
    #endif
    return true;
  }
  #ifdef SKETCH_DEBUG
  BookWorm.printf("\tFile Not Found: %s\r\n", path.c_str());   // If the file doesn't exist, return false
  #endif
  return false;
}

// converts a server arg to integer if possible
bool readServerArg(int argNum, signed int* result)
{
  String argVal = server.arg(argNum);
  char* str = (char*)argVal.c_str();
  char* end;
  double dv = strtod(str, &end);
  if (str == end)
  {
    return false;
  }
  signed int v = lround(dv);
  *result = v;
  return true;
}

// generates the start of a config item on the config page, the label and the form element
void generateConfigItemStart(const char* label, const char* id)
{
  server.sendContent("<form action='setconfig' method='post' onsubmit='return validateform(\"");
  server.sendContent(id);
  server.sendContent("\")' id='form_");
  server.sendContent(id);
  server.sendContent("' name='form_");
  server.sendContent(id);
  server.sendContent("' >");
  server.sendContent("<tr><td class='tbl_label'>");
  server.sendContent(label);
  server.sendContent(":</td><td class='tbl_field'>");
}

// generates the end of a config item on the config page, the submit button
void generateConfigItemEnd()
{
  server.sendContent("</td><td class='tbl_submit'><input type='submit' class='btn_submit' value='set' /></td></tr></form>\n");
}

// generates the input field of a config item on the config page
void generateConfigItemTxt(const char* label, const char* id, const char* type, const char* value, const char* other, const char* note) {
  generateConfigItemStart(label, id);
  server.sendContent("<input id='");
  server.sendContent(id);
  server.sendContent("' name='");
  server.sendContent(id);
  server.sendContent("' type='");
  server.sendContent(type);
  server.sendContent("' value='");
  server.sendContent(value);
  server.sendContent("' placeholder='");
  server.sendContent(value);
  server.sendContent("' ");
  if (other != NULL) {
    server.sendContent(other);
  }
  server.sendContent(" />");
  if (note != NULL && strlen(note) > 0) {
    server.sendContent("<br />");
    server.sendContent(note);
  }
  generateConfigItemEnd();
}

void serverClientStop()
{
  server.sendContent(""); // solves a bug involving invalid encoding due to improper chunking
  server.client().stop(); // stop() is needed if we sent no content length
}

void serveBasicHeader()
{
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
}


