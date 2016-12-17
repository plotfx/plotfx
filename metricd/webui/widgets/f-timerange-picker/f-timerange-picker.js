/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2016 Laura Schlimmer
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

var TimeRangePickerUtil = this.TimeRangePickerUtil || {};

/**
  * Format to a date time string yyyy-mm-dd hh:mm
  * @param timestamp to format
  * @param timezone determines the timezone (must be utc or local, defaults to local)
**/
TimeRangePickerUtil.formatDateTime = function(timestamp, timezone) {
  function appendLeadingZero(num) {
    if (num < 10) {
      return "0" + num;
    }

    return "" + num;
  }

  var d;
  if (timezone == 'utc') {
    d = new Date(TimeRangePickerUtil.toLocal(timestamp));
  } else {
    d = new Date(timestamp);
  }
  return [
      d.getFullYear(), "-",
      appendLeadingZero(d.getMonth() + 1), "-",
      appendLeadingZero(d.getDate()), " ",
      appendLeadingZero(d.getHours()), ":",
      appendLeadingZero(d.getMinutes())].join("");
}

TimeRangePickerUtil.toUTC = function(timestamp) {
  var date = new Date(timestamp);
  console.log(timestamp);
  return timestamp - (date.getTimezoneOffset() * dateUtil.kMillisPerMinute);
}

TimeRangePickerUtil.toLocal = function(timestamp) {
  var date = new Date(timestamp);
  return timestamp + (date.getTimezoneOffset() * dateUtil.kMillisPerMinute);
}

/**
  * The time range picker widget handler
  * @param timerange is the shared timerange object
  * @param widget is the widget html elem
  */
var TimeRangePickerWidget = function(timerange, timezone, widget) {
  'use strict';

  var submit_callbacks = [];

  this.setSubmitCallback = function(c) {
    submit_callbacks.push(c);
  }

  this.toggleVisibility = function() {
    if (widget.classList.contains('active')) {
      close();
    } else {
      show();
    }
  }

/********************************* private **********************************/

  var show = function() {
    render();
    widget.classList.add('active');
  }

  var close = function() {
    widget.classList.remove('active');
  }

  var render = function() {
    var tpl = templateUtil.getTemplate("f-timerange-picker-widget-tpl");
    var inner = widget.querySelector(".inner");
    DomUtil.clearChildren(inner);
    inner.appendChild(tpl.cloneNode(true));

    watchTimerangeButtons();
    watchCustomSubmit();
    enforceInputFormat(widget.querySelector("input[name='start']"));

    var elem;
    var range;

    /** check if current timerange is predefined **/
    var now = Date.now();
    if (now - timerange.end < 30 * dateUtil.kMillisPerSecond) {
      range = timerange.end - timerange.start;
      elem = widget.querySelector(
          "ul.timeranges button[data-value='" + range + "']");
    }

    /** custom timerange **/
    if (!elem) {
      range = 'custom';
      enableCustomInput();
      elem = widget.querySelector("ul.timeranges button[data-value='custom']");
    }

    updateInput(range);
    elem.classList.add("active");
  }

  var watchTimerangeButtons = function() {
    /** watch buttons with predefined timeranges */
    var buttons = widget.querySelectorAll("ul.timeranges li button.defined");
    for (var i = 0; i < buttons.length; i++) {

      buttons[i].addEventListener("click", function(e) {
        switchActiveButton(this);
        submitPredefined(this);
      }, false);

    }

    /** watch custom time range buttons **/
    var custom_buttons = {
      btn: widget.querySelector("button.custom"),
      link: widget.querySelector("a.custom")
    }
    for (var k in custom_buttons) {

      custom_buttons[k].addEventListener("click", function(e) {
        switchActiveButton(custom_buttons.btn);
        enableCustomInput();
      }, false);

    }
  }

  var watchCustomSubmit = function() {
    widget.querySelector(".custom form").addEventListener("submit", function(e) {
      e.preventDefault();
      submitCustom();
    }, false);
  }

  var switchActiveButton = function(btn) {
    widget.querySelector("ul.timeranges li button.active").
        classList.remove("active");

    btn.classList.add("active");
  }

  var updateInput = function(selected_timerange) {
    var start;
    var end;
    if (selected_timerange == 'custom') {
      start = timerange.start;
      end = timerange.end;

      enableCustomInput();

    } else {
      end = Date.now();
      start = end - parseInt(selected_timerange);
    }

    widget.querySelector(".custom input[name='start']").
        value = TimeRangePickerUtil.formatDateTime(start, timezone);

    widget.querySelector(".custom input[name='end']").
        value = TimeRangePickerUtil.formatDateTime(end, timezone);
  }

  var enableCustomInput = function() {
    var elem = widget.querySelector(".col.custom");
    elem.classList.add("active");

    var start_input = elem.querySelector("input[name='start']");
    start_input.removeAttribute('readonly');
    start_input.focus();

    elem.querySelector("input[name='end']").removeAttribute('readonly');

  }

  var enforceInputFormat = function(input) {
    //TODO!
  }

  var disableCustomInput = function() {
    var elem = widget.querySelector(".col.custom");
    elem.classList.remove("active");

    elem.querySelector("input[name='start']").
        setAttribute('readonly', true);

    elem.querySelector("input[name='end']").
        setAttribute('readonly', true);
  }

  var submitPredefined = function(elem) {
    var selected_range = {};
    selected_range.end = Date.now();
    selected_range.start =
        selected_range.end - parseInt(elem.getAttribute('data-value'));

    submit(selected_range);
  }

  var submitCustom = function() {
    var selected_range = {
      start: TimeRangePickerUtil.toUTC(new Date(widget.querySelector("input[name='start']").value).getTime()),
      end: TimeRangePickerUtil.toUTC(new Date(widget.querySelector("input[name='end']").value).getTime())
    };

    submit(selected_range);
  }

  var submit = function(selected_range) {
    submit_callbacks.forEach(function(callback) {
      callback(selected_range);
    });

    close();
  }

  /** initialize **/
  widget.querySelector("button.cancel").addEventListener("click", function() {
    close();
  }, false);

  /** close widget on ESC keypress **/
  document.addEventListener("keydown", function(e) {
    if (e.keyCode == 27) {
      close();
    }
  }, false);
}

var TimeRangePickerComponent = function() {
  'use strict';

  /** default config **/
  var config = {
    range: 5 * dateUtil.kMillisPerMinute, //5 minutes
    timezone: 'utc'
  }

  var timerange = {};
  timerange.end = Date.now();
  timerange.start = timerange.end - config.range;

  var this_;
  var widget;

  this.createdCallback = function() {
    var tpl = templateUtil.getTemplate("f-timerange-picker-tpl");
    this.appendChild(tpl);

    this_ = this;

    initializeWidget();
    watchTimerangeMover();
    watchInputClick();

    updateInputValue();
  }

  this.setTimezone = function(timezone) {
    /** allow only supported timezones **/
    if (timezone != 'utc' && timezone != 'local') {
      return;
    }

    config.timezone = timezone;
  }

  /**
    * Set the start and end of the selected timerange
    * @param start timestamp in milliseconds
    * @param end timestamp in milliseconds
  **/
  this.setTimerange = function(start, end) {
    timerange.start = start;
    timerange.end = end;

    updateInputValue();
  }

  /**
    * Returns start and end of the selected timerange
  **/
  this.getTimerange = function() {
    return {
      start: timerange.start,
      end: timerange.end
    }
  }

/******************************** private *************************************/

  var initializeWidget = function() {
    widget = new TimeRangePickerWidget(
        timerange,
        config.timezone,
        this_.querySelector(".widget"));
    widget.setSubmitCallback(function(new_timerange) {
      timerange.start = new_timerange.start;
      timerange.end = new_timerange.end;
      fireSubmitEvent();
    });

    document.addEventListener("click", function(e) {
      widget.toggleVisibility();
    }, false);

    this_.addEventListener("click", function(e) {
      e.stopPropagation();
    }, false);
  }

  var watchTimerangeMover = function() {
    this_.querySelector(".arrow_left").addEventListener("click", function(e) {
      moveTimerange(config.range * -1);
    }, false);

    this_.querySelector(".arrow_right").addEventListener("click", function(e) {
      moveTimerange(config.range);
    }, false);
  }

  var watchInputClick = function() {
    this_.querySelector("input").addEventListener("click", function(e) {
      widget.toggleVisibility(timerange);
    }, false);
  }

  var moveTimerange = function(range) {
    timerange.start += range;
    timerange.end += range;

    updateInputValue();
    fireSubmitEvent();
  }

  var updateInputValue = function() {
    var input = this_.querySelector("input");
    input.value = TimeRangePickerUtil.formatDateTime(timerange.start, config.timezone) +
        " - " +
        TimeRangePickerUtil.formatDateTime(timerange.end, config.timezone);
  }

  var fireSubmitEvent = function() {
    var ev = new CustomEvent('submit');
    this_.dispatchEvent(ev);
  }
}


var proto = Object.create(HTMLElement.prototype);
TimeRangePickerComponent.apply(proto);
document.registerElement("f-timerange-picker", { prototype: proto });
