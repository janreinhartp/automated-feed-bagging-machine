(function () {
  var page = document.body.getAttribute("data-page");
  var passwordKey = "feedWebPassword";

  function password() {
    return sessionStorage.getItem(passwordKey) || "";
  }

  function authHeaders() {
    var headers = { "Content-Type": "application/json" };
    if (password()) {
      headers.Authorization = "Basic " + btoa("operator:" + password());
    }
    return headers;
  }

  function request(path, options) {
    options = options || {};
    options.headers = Object.assign(authHeaders(), options.headers || {});
    return fetch(path, options).then(function (response) {
      return response.json().then(function (body) {
        body.httpStatus = response.status;
        return body;
      });
    });
  }

  function show(id, text) {
    var node = document.getElementById(id);
    if (node) {
      node.textContent = text;
    }
  }

  function renderStatus(status) {
    show("state", status.state || "unknown");
    show("reason", status.inhibit_reason || "");
    show("weight", status.weight_g === null || status.weight_g === undefined ? "unavailable" : status.weight_g + " g");
    show("motion", status.motion_permitted ? "permitted" : "inhibited");
    show("uptime", status.uptime_ms === undefined ? "" : Math.floor(status.uptime_ms / 1000) + " s since boot");
    show("protocols", "Weighing map " + (status.weighing_protocol_verified ? "verified" : "unverified") +
      " · I/O map " + (status.io_protocol_verified ? "verified" : "unverified"));
    show("buses", "Weight " + status.weighing_baud + " baud, slave " + status.weighing_slave +
      " · I/O " + status.io_baud + " baud, slave " + status.io_slave);
    var banner = document.getElementById("banner");
    if (banner) {
      banner.hidden = false;
      banner.textContent = status.password_change_required
        ? "Factory web password is still in use. Change it on the settings page before commissioning."
        : "Motion is inhibited. The filling sequence is not in this firmware, and device manuals are not commissioned.";
    }
  }

  function connectStatus() {
    if (!document.getElementById("state")) {
      return;
    }
    function poll() {
      request("/api/status").then(renderStatus).catch(function () {
        show("state", "offline");
      });
    }
    poll();
    var socket = null;
    try {
      socket = new WebSocket((location.protocol === "https:" ? "wss://" : "ws://") + location.host + "/ws");
      socket.onmessage = function (event) {
        renderStatus(JSON.parse(event.data));
      };
      socket.onerror = function () {
        window.setInterval(poll, 1000);
      };
    } catch (error) {
      window.setInterval(poll, 1000);
    }
  }

  function bindLogin() {
    var form = document.getElementById("login");
    if (!form) {
      return;
    }
    form.addEventListener("submit", function (event) {
      event.preventDefault();
      sessionStorage.setItem(passwordKey, document.getElementById("web-password").value);
      show("result", "Password stored for this browser tab.");
    });
  }

  function fillSettings(data) {
    Object.keys(data).forEach(function (key) {
      var input = document.getElementById(key);
      if (input && typeof data[key] !== "object") {
        input.value = data[key];
      }
    });
  }

  function readForm(form) {
    var body = {};
    Array.prototype.forEach.call(form.elements, function (element) {
      if (!element.name || element.value === "") {
        return;
      }
      body[element.name] = element.type === "number" ? Number(element.value) : element.value;
    });
    return body;
  }

  if (page === "settings") {
    request("/api/settings").then(fillSettings);
    document.getElementById("settings-form").addEventListener("submit", function (event) {
      event.preventDefault();
      request("/api/settings", { method: "POST", body: JSON.stringify(readForm(event.target)) }).then(function (body) {
        show("result", body.ok ? "Saved. Motion was not changed." : "Rejected (" + body.error + ").");
      });
    });
    document.getElementById("password-form").addEventListener("submit", function (event) {
      event.preventDefault();
      request("/api/password", {
        method: "POST",
        body: JSON.stringify({ next: document.getElementById("next-password").value })
      }).then(function (body) {
        if (body.ok) {
          sessionStorage.setItem(passwordKey, document.getElementById("next-password").value);
        }
        show("result", body.ok ? "Web password changed." : "Password rejected.");
      });
    });
    document.getElementById("network-form").addEventListener("submit", function (event) {
      event.preventDefault();
      request("/api/network", { method: "POST", body: JSON.stringify(readForm(event.target)) }).then(function (body) {
        show("result", body.ok ? "Network settings stored. Restart the controller to apply the access point." : "Network settings rejected.");
      });
    });
  }

  if (page === "calibration") {
    request("/api/calibration").then(function (data) {
      ["closed_pulse_us", "open_pulse_us", "min_safe_pulse_us", "max_safe_pulse_us"].forEach(function (key) {
        document.getElementById("feeder_" + key).value = data.feeder_gate[key];
        document.getElementById("discharge_" + key).value = data.discharge_gate[key];
      });
      document.getElementById("zero_offset_counts").value = data.weighing.zero_offset_counts;
      document.getElementById("span_counts").value = data.weighing.span_counts;
      document.getElementById("span_grams").value = data.weighing.span_grams;
    });
    document.getElementById("calibration-form").addEventListener("submit", function (event) {
      event.preventDefault();
      var body = {
        calibrated: false,
        feeder_gate: {
          closed_pulse_us: Number(document.getElementById("feeder_closed_pulse_us").value),
          open_pulse_us: Number(document.getElementById("feeder_open_pulse_us").value),
          min_safe_pulse_us: Number(document.getElementById("feeder_min_safe_pulse_us").value),
          max_safe_pulse_us: Number(document.getElementById("feeder_max_safe_pulse_us").value)
        },
        discharge_gate: {
          closed_pulse_us: Number(document.getElementById("discharge_closed_pulse_us").value),
          open_pulse_us: Number(document.getElementById("discharge_open_pulse_us").value),
          min_safe_pulse_us: Number(document.getElementById("discharge_min_safe_pulse_us").value),
          max_safe_pulse_us: Number(document.getElementById("discharge_max_safe_pulse_us").value)
        },
        weighing: {
          zero_offset_counts: Number(document.getElementById("zero_offset_counts").value),
          span_counts: Number(document.getElementById("span_counts").value),
          span_grams: Number(document.getElementById("span_grams").value)
        }
      };
      request("/api/calibration", { method: "POST", body: JSON.stringify(body) }).then(function (result) {
        show("result", result.ok ? "Draft stored. It is not marked calibrated and it does not move the servos." : "Draft rejected.");
      });
    });
  }

  if (page === "manual") {
    document.getElementById("start").addEventListener("click", function () {
      request("/api/command", { method: "POST", body: JSON.stringify({ command: "start_cycle" }) }).then(function (body) {
        show("result", body.error === "sequence_not_implemented" ? "Start rejected: the filling sequence is not implemented." : "Command rejected.");
      });
    });
  }

  if (page === "diagnostics") {
    function load() {
      request("/api/diagnostics").then(function (body) {
        var lines = (body.events || []).map(function (event) {
          return event.time_ms + " ms  " + event.code_name + "  " + event.description;
        });
        show("events", lines.length ? lines.join("\n") : "No events yet.");
      });
    }
    load();
    window.setInterval(load, 2000);
  }

  bindLogin();
  connectStatus();
})();
