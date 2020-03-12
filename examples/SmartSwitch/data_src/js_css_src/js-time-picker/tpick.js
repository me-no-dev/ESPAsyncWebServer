var tpick = {
  attach : function (container, target) {
  // attach() : attach time picker to target

    // Generate a unique random ID for the time picker
    var uniqueID = 0;
    while (document.getElementById("tpick-" + uniqueID) != null) {
      uniqueID = Math.floor(Math.random() * (100 - 2)) + 1;
    }

    // Create new time picker
    var tp = document.createElement("div");
    tp.id = "tpick-" + uniqueID;
    tp.dataset.target = target;
    tp.classList.add("tpicker");

    // Create hour picker
    tp.appendChild(this.draw("h"));
    tp.appendChild(this.draw("m"));
    tp.appendChild(this.draw("ap"));

    // OK button
    var bottom = document.createElement("div"),
        ok = document.createElement("input");
    ok.setAttribute("type", "button");
    ok.value = "OK";
    ok.addEventListener("click", function(){ tpick.set(this); });
    bottom.classList.add("tpicker-btn");
    bottom.appendChild(ok);
    tp.appendChild(bottom);

    // Attach time picker to target container
    document.getElementById(container).appendChild(tp);
  },

  draw : function (type) {
  // draw() : support function to create the hr, min, am/pm selector

    // Create the controls
    var docket = document.createElement("div"),
        up = document.createElement("div"),
        down = document.createElement("div"),
        text = document.createElement("input");
    docket.classList.add("tpicker-" + type);
    up.classList.add("tpicker-up");
    down.classList.add("tpicker-down");
    up.innerHTML = "&#65087;";
    down.innerHTML = "&#65088;";
    text.readOnly = true;
    text.setAttribute("type", "text");

    // Default values + click event
    // You can do your own modifications here
    if (type=="h") { 
      text.value = "12"; 
      up.addEventListener("click", function(){ tpick.spin("h", 1, this); });
      down.addEventListener("click", function(){ tpick.spin("h", 0, this); });
    } else if (type=="m") { 
      text.value = "10";
      up.addEventListener("click", function(){ tpick.spin("m", 1, this); });
      down.addEventListener("click", function(){ tpick.spin("m", 0, this); });
    } else {
      text.value = "AM";
      up.addEventListener("click", function(){ tpick.spin("ap", 1, this); });
      down.addEventListener("click", function(){ tpick.spin("ap", 0, this); });
    }

    // Complete + return the docket
    docket.appendChild(up);
    docket.appendChild(text);
    docket.appendChild(down);
    return docket;
  },

  spin : function (type, direction, el) {
  // spin() : when the up/down button is pressed

    // Get current field + value
    var parent = el.parentElement,
        field = parent.getElementsByTagName("input")[0],
        value = field.value;

    // Spin it
    if (type=="h") {
      value = parseInt(value);
      if (direction) { value++; } else { value--; }
      if (value==0) { value = 12; }
      else if (value>12) { value = 1; }
    } else if (type=="m") {
      value = parseInt(value);
      if (direction) { value+=5; } else { value-=5; }
      if (value<0) { value = 55; }
      else if (value>60) { value = 0; }
      if (value<10) { value = "0" + value; }
    }
    else {
      value = value=="PM" ? "AM" : "PM";
    }
    field.value = value;
  },

  set : function (el) {
  // set() : set the selected time on the target

    // Get the parent container
    var parent = el.parentElement;
    while (parent.classList.contains("tpicker") == false) {
      parent = parent.parentElement;
    }

    // Formulate + set selected time
    var input = parent.querySelectorAll("input[type=text]");
    var time = input[0].value + ":" + input[1].value + " " + input[2].value;
    document.getElementById(parent.dataset.target).value = time;
  }
};