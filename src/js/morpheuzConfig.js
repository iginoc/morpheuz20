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

(function() {
  'use strict';

  var MorpheuzConfig = {};

  /*
   * Constants
   */
  MorpheuzConfig.mConst = function() {
    var urlPrefix = "http://ui.morpheuz.net/morpheuz/";
    return {
      limit : 60,
      divisor : 600000,
      url : urlPrefix + "view-",
      versionDef : "0",
      lowestVersion : 22,
      smartDef : "N",
      fromhrDef : "6",
      fromminDef : "30",
      tohrDef : "7",
      tominDef : "15",
      ctrlTransmitDone : 1,
      ctrlVersionDone : 2,
      ctrlGoneOffDone : 4,
      ctrlDoNext : 8,
      ctrlSetLastSent : 16,
      ctrlLazarus : 32,
      ctrlSnoozesDone : 64,
      displayDateFmt : "WWW, NNN dd, yyyy hh:mm",
      timeout : 4000,
      urlNotReady : urlPrefix + "view-not-ready.html"
    };
  };
  /*
   * Language strings
   */
  MorpheuzConfig.mLang = function() {
    return {
      ok : "OK",
      sending : "Sending",
      disabled : "Disabled",
      cnc : "Could not calculate",
      sa : "Smart Alarm",
      startM : "Start Morpheuz",
      bedTime : "Bed time",
      bedNow : "Bed Now",
      cancelBed : "Cancel bed time",
      earliest : "Earliest: ",
      latest : "Latest: ",
      suggested : "Suggested",
      automatic : "Automatic",
      bedtimeIn30Mins : "Bed time",
      summary : "Morpheuz Summary",
      showChart : "Show Chart"
    };
  };

  module.exports = MorpheuzConfig;

}());
