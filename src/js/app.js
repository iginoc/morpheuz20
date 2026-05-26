/* 
 * Morpheuz Sleep Monitor
 *
 * Copyright (c) 2013-2016 James Fowler
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* global window */

(function() {
  'use strict';

  var MorpheuzUtil = require("./morpheuzUtil");
  var MorpheuzConfig = require("./morpheuzConfig");
  var MorpheuzCommon = require("./morpheuzCommon");

  var Clay = require('pebble-clay');
  var clayConfig = require('./config_clay');
  var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

  /*
   * Reset log
   */
  function resetWithPreserve() {
    console.log("resetWithPreserve");

    // Define keep list
    var keep = [ {
      n : "version",
      d : MorpheuzConfig.mConst().versionDef
    }, {
      n : "autoReset",
      d : "0"
    }, {
      n : "base",
      d : "0"
    }, {
      n : "haurl",
      d : ""
    }, {
      n : "hatoken",
      d : ""
    }, {
      n : "steps",
      d : "0"
    }, {
      n : "clay-settings",
      d : null
    } ];

    // Remember the keep list
    for (var i = 0; i < keep.length; i++) {
      var ki = keep[i];
      ki.v = MorpheuzUtil.getNoDef(ki.n);
    }

    // Clear memory
    window.localStorage.clear();
    clearPoints();

    // Restore from keep list
    for (var j = 0; j < keep.length; j++) {
      var kj = keep[j];
      if (kj.v !== null && kj.v !== "null") {
        MorpheuzUtil.setWithDef(kj.n, kj.v, kj.d);
      }
    }
  }

  /*
   * There have been various reports of data not being cleared for the next day
   * This is about as explicit as can be.
   */
  function clearPoints() {
    for (var i = 0; i < MorpheuzConfig.mConst().limit; i++) {
      var entry = "P" + i;
      MorpheuzUtil.setNoDef(entry, "-1");
    }
  }

  /*
   * Store data returned from the watch
   */
  function storePointInfo(point, biggest) {
    var entry = "P" + point;
    if (biggest === 0) // Don't pass -1 across the link but 0 really means null
      biggest = -1; // Null
    else if (biggest === 5000)
      biggest = -2; // Ignored by user
    MorpheuzUtil.setNoDef(entry, biggest);
  }

  /*
   * Process ready from the watch
   */
  Pebble.addEventListener("ready", function(e) {
    console.log("ready");

    var smartStr = MorpheuzUtil.getNoDef("smart");
    if (smartStr === null || smartStr === "null") {
      resetWithPreserve();
    }

    setInterval(transmitHA, 5000);
  });

  /*
   * Return a control value back ACK
   */
  function callWatchApp(ctrlVal) {
    function decodeKeyCtrl(ctrlVal, keyVal, name) {
      return (ctrlVal & keyVal) ? name + " " : "";
    }
    console.log("ACK " + decodeKeyCtrl(ctrlVal, MorpheuzConfig.mConst().ctrlTransmitDone, "ctrlTransmitDone") + decodeKeyCtrl(ctrlVal, MorpheuzConfig.mConst().ctrlVersionDone, "ctrlVersionDone") + decodeKeyCtrl(ctrlVal, MorpheuzConfig.mConst().ctrlGoneOffDone, "ctrlGoneOffDone") + decodeKeyCtrl(ctrlVal, MorpheuzConfig.mConst().ctrlSnoozesDone, "ctrlSnoozesDone") + decodeKeyCtrl(ctrlVal, MorpheuzConfig.mConst().ctrlDoNext, "ctrlDoNext") + decodeKeyCtrl(ctrlVal, MorpheuzConfig.mConst().ctrlSetLastSent, "ctrlSetLastSent") + decodeKeyCtrl(ctrlVal, MorpheuzConfig.mConst().ctrlLazarus, "ctrlLazarus"));
    Pebble.sendAppMessage({
      "keyCtrl" : ctrlVal
    });
  }

  /*
   * Process sample from the watch
   */
  Pebble.addEventListener("appmessage", function(e) {

    // Build a response for the watchapp
    var ctrlVal = 0;

    // Incoming version number
    if (typeof e.payload.keyVersion !== "undefined") {
      var version = parseInt(e.payload.keyVersion, 10);
      console.log("MSG version=" + version);
      MorpheuzUtil.setNoDef("version", version);
      ctrlVal = ctrlVal | MorpheuzConfig.mConst().ctrlVersionDone;
    }

    // Incoming origin timestamp - this is a reset
    if (typeof e.payload.keyBase !== "undefined") {
      var base = parseInt(e.payload.keyBase, 10);
      console.log("MSG base (watch)=" + base);
      // Watch delivers local time in seconds...
      base = base * 1000;
      resetWithPreserve();
      MorpheuzUtil.setNoDef("base", base);
      ctrlVal = ctrlVal | MorpheuzConfig.mConst().ctrlDoNext | MorpheuzConfig.mConst().ctrlSetLastSent;
    }

    // Ricezione passi
    if (typeof e.payload.keySteps !== "undefined") {
      var steps = parseInt(e.payload.keySteps, 10);
      console.log("MSG steps=" + steps);
      MorpheuzUtil.setNoDef("steps", steps);
      ctrlVal = ctrlVal | MorpheuzConfig.mConst().ctrlDoNext;
    }

    // Incoming data point
    if (typeof e.payload.keyPoint !== "undefined") {
      var point = parseInt(e.payload.keyPoint, 10);
      var top = point >> 16;
      var bottom = point & 0xFFFF;
      console.log("MSG point=" + top + ", biggest=" + bottom);
      storePointInfo(top, bottom);
      ctrlVal = ctrlVal | MorpheuzConfig.mConst().ctrlDoNext | MorpheuzConfig.mConst().ctrlSetLastSent;
    }

    // Store the snoozes
    if (typeof e.payload.keySnoozes !== "undefined") {
      var snoozes = parseInt(e.payload.keySnoozes, 10);
      console.log("MSG snoozes=" + snoozes);
      MorpheuzUtil.setNoDef("snoozes", snoozes);
      ctrlVal = ctrlVal | MorpheuzConfig.mConst().ctrlSnoozesDone | MorpheuzConfig.mConst().ctrlDoNext;
    }
    
    // Incoming transmit to automatics
    if (typeof e.payload.keyTransmit !== "undefined") {
      console.log("MSG transmit");
      transmitMethods();
      ctrlVal = ctrlVal | MorpheuzConfig.mConst().ctrlTransmitDone;
    }

    // Fault detected
    if (typeof e.payload.keyFault !== "undefined") {
      var faultCode = parseInt(e.payload.keyFault, 10);
      console.log("MSG fault=" + faultCode);
      MorpheuzUtil.setNoDef("fault", faultCode);
      ctrlVal = ctrlVal | MorpheuzConfig.mConst().ctrlDoNext;
    }

    // What auto reset are we doing today?
    if (typeof e.payload.keyAutoReset !== "undefined") {
      var autoReset = parseInt(e.payload.keyAutoReset, 10);
      console.log("MSG keyAutoReset=" + autoReset);
      MorpheuzUtil.setNoDef("autoReset", autoReset);
      ctrlVal = ctrlVal | MorpheuzConfig.mConst().ctrlDoNext | MorpheuzConfig.mConst().ctrlSetLastSent;
    }

    // Respond back to watchapp here - we need assured positive delivery -
    // cannot
    // trust that it has reached the phone - must make
    // sure it has reached and been processed by the Pebble App and Javascript
    if (ctrlVal !== 0) {
      callWatchApp(ctrlVal);
    }

  });

  /*
   * Invio dati a MQTT Bridge o Home Assistant Webhook
   */
  function transmitHA() {
    var url = MorpheuzUtil.getNoDef("haurl");
    var token = MorpheuzUtil.getNoDef("hatoken");

    if (!url || url === "" || url === "null") { console.log("HA: URL non configurato. Invio annullato."); return; }
    if (!token || token === "" || token === "null") { console.log("HA: Token non configurato. Invio annullato."); return; }

    // Pulizia URL: rimuove spazi e slash finale, assicura il protocollo http
    console.log("HA: URL letto: '" + url + "'");
    console.log("HA: Token letto (parziale per sicurezza): '" + token.substring(0, 5) + "...'");
    url = url.trim();
    if (url.charAt(url.length - 1) === '/') {
      url = url.substring(0, url.length - 1);
    }
    if (url.indexOf('http') !== 0) {
      url = 'http://' + url;
    }

    console.log("HA: URL dopo la pulizia: '" + url + "'");
    console.log("HA: Token dopo la pulizia (parziale per sicurezza): '" + token.substring(0, 5) + "...'");
    // Pulizia Token: rimuove spazi, ritorni a capo e caratteri invisibili
    token = token.replace(/(\r\n|\n|\r)/gm, "").trim();

    // Recupero dati con valori di fallback per evitare 'null'
    var base = MorpheuzUtil.getWithDef("base", 0);
    var goneOff = MorpheuzUtil.getWithDef("goneOff", "N");
    var snoozes = MorpheuzUtil.getWithDef("snoozes", 0);
    var steps = MorpheuzUtil.getWithDef("steps", 0);

    var deep_mins = 0;
    var light_mins = 0;
    var awake_mins = 0;
    for (var i = 0; i < MorpheuzConfig.mConst().limit; i++) {
      var p = MorpheuzUtil.getNoDef("P" + i);
      if (p !== null) {
        var val = parseInt(p, 10);
        if (val > 0) { // Escludiamo i valori nulli (-1) e ignorati (-2)
          if (val <= 40) { // DEEP_SLEEP_THRESHOLD (come definito nel C)
            deep_mins += 10;
          } else if (val < 120) { // LIGHT_ABOVE (come definito nel C)
            light_mins += 10;
          } else {
            awake_mins += 10;
          }
        }
      }
    }

    var endpoint = url + "/api/states/sensor.igi_activity";

    var payload = {
      state: parseInt(steps, 10), // Lo stato principale ora mostra il numero di passi come numero
      attributes: {
        unique_id: "morpheuz_igi_activity_monitor",
        base: parseInt(base, 10),
        goneOff: goneOff,
        snoozes: parseInt(snoozes, 10),
        deep_sleep_min: deep_mins,
        light_sleep_min: light_mins,
        awake_min: awake_mins,
        total_sleep_min: deep_mins + light_mins,
        steps: parseInt(steps, 10),
        exptime: new Date().toISOString(),
        friendly_name: "Igi Activity Monitor",
        unit_of_measurement: "steps"
      }
    };

    console.log("HA: Invio dati a " + endpoint + " con payload: " + JSON.stringify(payload));
    var req = new XMLHttpRequest();
    req.open('POST', endpoint, true);
    req.setRequestHeader('Content-Type', 'application/json');
    req.setRequestHeader("Authorization", "Bearer " + token);

    req.onload = function() {
        if (req.status === 200 || req.status === 201) {
            console.log("HA Update Success: " + req.status);
        } else {
            console.log("HA Update Failed: " + req.status + " " + req.responseText);
        }
    };

    req.onerror = function() {
        console.log("HA Network Error");
    };

    req.send(JSON.stringify(payload));
  }

  /*
   * Transmit method list
   */
  function transmitMethods() {

    // Sends
    transmitHA();

    // Protect and report time
    MorpheuzUtil.setNoDef("transmitDone", "done");
    MorpheuzUtil.setNoDef("exptime", new Date().format(MorpheuzConfig.mConst().displayDateFmt));
  }

  Pebble.addEventListener("webviewclosed", function(e) {
    if (e && !e.response) return;
    var response = clay.getSettings(e.response, false);
    var settings = {};
    Object.keys(response).forEach(function(key) {
      settings[key] = response[key].value;
    });

    // Mappa le impostazioni di Clay nel formato utilizzato da Morpheuz
    MorpheuzUtil.setNoDef("haurl", settings.haurl);
    MorpheuzUtil.setNoDef("hatoken", settings.hatoken);
  });

  /*
   * Show the config/display page - this will show a graph and allow a reset
   */
  Pebble.addEventListener("showConfiguration", function(e) {
    var settings = {
      "haurl": MorpheuzUtil.getNoDef("haurl"),
      "hatoken": MorpheuzUtil.getNoDef("hatoken")
    };
    console.log("Opening Clay with settings: " + JSON.stringify(settings));
    Pebble.openURL(clay.generateUrl(settings));
  });

  /*
   * Unclear if this serves a purpose at all
   */
  Pebble.addEventListener("configurationClosed", function(e) {
    console.log("configurationClosed");
  });

}());
