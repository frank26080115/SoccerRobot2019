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
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  #ifdef SKETCH_DEBUG
  BookWorm.printf("handleFileRead: %s\r\n", path.c_str());
  #endif
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
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

bool readServerArg(int argNum, signed int* result)
{
  String argVal = server.arg(argNum);
  char* str = (char*)argVal.c_str();
  char* end;
  signed int v = strtol(str, &end, 10);
  if (str == end)
  {
    return false;
  }
  *result = v;
  return true;
}

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

void generateConfigItemEnd()
{
  server.sendContent("</td><td class='tbl_submit'><input type='submit' value='set' height='100%' /></td></tr></form>\n");
}

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

