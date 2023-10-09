/*
 * This file and associated .cpp file are licensed under the MIT Lesser General Public License Copyright (c) 2023 RoboRuckus Group
 * 
 * External libraries needed:
 * ESPAsyncWebServer: https://github.com/esphome/ESPAsyncWebServer
 * 
 * Contributors: Sam Groveman
 */

#pragma once
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <Configuration.h>
#include <CommandProcessor.h>

/// @brief Local web server.
class Webserver {
    public:
        /// @brief Reboot on firmware update flag
        bool shouldReboot = false;
        
        Webserver(Configuration* Config, CommandProcessor* Command, AsyncWebServer* webserver);
        void ServerStart();
        void ServerStop();
        
    private:
        AsyncWebServer* server;
        Configuration* config;
        CommandProcessor* command;
        static void onUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

        /// @brief Text of update webpage part 1
        const char indexPage_Part1[187] = "<!DOCTYPE html>\
        <html lang='en-us'>\
        <head>\
        <title>Firmware Updater</title>\
        </head>\
        <body>\
        <div id='up-wrap'>\
        <h1 id='hubName' data-name='";

        /// @brief Text of update webpage part 2
        const char indexPage_Part2[3442] = "'></h1>\
        <h2>Upload Firmware</h2>\
        <div id='up-progress'>\
            <div id='up-bar'></div>\
            <div id='up-percent'>0%</div>\
        </div>\
        <input type='file' id='up-file' disabled />\
        <label for='up-file' id='up-label'>\
            Update\
        </label>\
        <div id='message'></div>\
        </div>\
        <script>\
        document.addEventListener('DOMContentLoaded', (e) => {\
        document.getElementById('hubName').innerHTML = decodeURI(document.getElementById('hubName').getAttribute('data-name'));\
        });\
        var uprog = {\
            hBar : null,\
            hPercent : null,\
            hFile : null,\
            init : () => {\
                uprog.hBar = document.getElementById('up-bar');\
                uprog.hPercent = document.getElementById('up-percent');\
                uprog.hFile = document.getElementById('up-file');\
                uprog.hFile.disabled = false;\
                document.getElementById('up-label').onclick = uprog.upload;\
            },\
            update : (percent) => {\
            percent = percent + '%';\
            uprog.hBar.style.width = percent;\
            uprog.hPercent.innerHTML = percent;\
            if (percent == '100%') { uprog.hFile.disabled = false; }\
            },\
            upload : () => {\
            if(uprog.hFile.files.length == 0 ){\
            return;\
            }\
            let file = uprog.hFile.files[0];\
            uprog.hFile.disabled = true;\
            uprog.hFile.value = '';\
            let xhr = new XMLHttpRequest(), data = new FormData();\
            data.append('upfile', file);\
            xhr.open('POST', '/update');\
            let percent = 0, width = 0;\
            xhr.upload.onloadstart = (evt) => { uprog.update(0); };\
            xhr.upload.onloadend = (evt) => { uprog.update(100); };\
            xhr.upload.onprogress = (evt) => {\
                percent = Math.ceil((evt.loaded / evt.total) * 100);\
                uprog.update(percent);\
            };\
            xhr.onload = function () {\
                if (this.response != 'OK' || this.status != 200) {\
                document.getElementById('message').innerHTML = 'ERROR!';\
                } else {\
                uprog.update(100);\
                document.getElementById('message').innerHTML = 'Success, rebooting!';\
                }\
            };\
            xhr.send(data);\
            }\
        };\
        window.addEventListener('load', uprog.init);\
        </script>\
        <style>\
        #message{font-size:18px;font-weight:bolder}\
        #up-file,#up-label{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:17px}\
        #up-label{background:#f1f1f1;border:0;display:block;line-height:44px}\
        body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}\
        #up-file{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}\
        #up-bar,#up-progress{background-color:#f1f1f1;border-radius:10px;position:relative}\
        #up-bar{background-color:#3498db;width:0%;height:30px}\
        #up-wrap{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}\
        #up-label{background:#3498db;color:#fff;cursor:pointer}\
        #up-percent{position:absolute;top:6px;left:0;width:100%;display:flex;align-items:center;justify-content:center;text-shadow:-1px 1px 0 #000,1px 1px 0 #000,1px -1px 0 #000,-1px -1px 0 #000;color:#fff}</style>\
        </body>\
        </html>";
};