var param_json
var global_class_names
var base_url

function update_clicked() {
  base_url = "http://" + document.getElementById("boxip").value + ":8080";
  var img_src = base_url + "/support/live/rendered";
  var img = document.getElementById("livestream");
  img.src = img_src;

  //Update params
  url = base_url + "/get_current_params"
  xhr = new XMLHttpRequest();
  xhr.open("GET", url, true);
  xhr.onload = () => {
    var data = JSON.parse(xhr.response);
    param_json = data;
    ParamData_h = document.getElementById("ParamData");
    ParamData_h.value = JSON.stringify(data, null, 2);
    ParamData_h.style.height = "auto";
    ParamData_h.style.height = (ParamData_h.scrollHeight) + "px";

  }
  xhr.send();

  //Update belt speed
  url = base_url + "/robot/get-object?no=100"
  xhr_belt_data = new XMLHttpRequest();
  xhr_belt_data.open("GET", url, true);
  xhr_belt_data.onload = () => {
    var data = JSON.parse(xhr_belt_data.response);
    document.getElementById("beltspeed").value = data["v"];
    document.getElementById("beltstatus").value = data["bs"];
  }

  xhr_belt_data.send();


  //get class names
  url = base_url + "/v2/taxonomy/classes"
  xhr_class_names = new XMLHttpRequest();
  xhr_class_names.open("GET", url, true);
  xhr_class_names.onload = () => {
  var data = JSON.parse(xhr_class_names.response);
  global_class_names = data["taxonomy_list"];
  console.log("global_class_names: ", global_class_names);
  }
  xhr_class_names.send();

  //Populate the dropdown
  //Clear the dropdown
  var dropdown = document.getElementById("class_lists");
  while (dropdown.firstChild) {
    dropdown.removeChild(dropdown.firstChild);
  }

  if (global_class_names) {
    for (var i = 0; i < global_class_names.length; i++) {
      var checkbox = document.createElement("input");
      checkbox.type = "checkbox";
      checkbox.value = global_class_names[i];
      checkbox.id = "class_checkbox_" + i;
      var label = document.createElement("label");
      label.htmlFor = "class_checkbox_" + i;
      label.innerHTML = global_class_names[i];
      document.getElementById("class_lists").appendChild(checkbox);
      document.getElementById("class_lists").appendChild(label);
            // add a new line
      var newLine = document.createElement("br");
      document.getElementById("class_lists").appendChild(newLine);
    }
  }

}

var interval = setInterval(function() {
  var is_checked = document.getElementById("objectdata").checked;
  if(is_checked) {
    base_url = "http://" + document.getElementById("boxip").value + ":8080";
    url = base_url + "/robot/get-object?no=100"
    xhr = new XMLHttpRequest();
    xhr.open("GET", url, true);
    xhr.onload = () => {
      var data = xhr.responseText;
      //Dump data to text area
      textarea = document.getElementById("HistoryText");
      textarea.value = document.getElementById("HistoryText").value + "\n" + data;
      textarea.scrollTop = textarea.scrollHeight;
      
    }

    xhr.send();
  }
}, 1000);

function setButton_clicked () {
  base_url = "http://" + document.getElementById("boxip").value + ":8080";
  var url = base_url + "/set_param";

  current_params = document.getElementById("ParamData").value
  var current_params_json = JSON.parse(current_params);

  var changed_dictionary = {};

  // check if the params are changed
  //Iterate through each key in the json
  var is_changed = false;
  for (var key in current_params_json) {
    if (current_params_json.hasOwnProperty(key)) {
      if(current_params_json[key] != param_json[key]) {
        //add each changed pair to the dictionary
        changed_dictionary[key] = current_params_json[key];
        is_changed = true;
      }
    }
  }

  //print changed_dictionary to console
  //iterate through each key in the dictionary
  for (var key in changed_dictionary) {
    if (changed_dictionary.hasOwnProperty(key)) {
      console.log("Changed key: ", key, " value: ", changed_dictionary[key]);
      //populate the changed_json
      var changed_value = {param:key, value:changed_dictionary[key]};
      var body = JSON.stringify(changed_value);
      console.log("body: ", body);
      var xhr = new XMLHttpRequest();
      xhr.open("POST", url, true);
      xhr.setRequestHeader("Content-Type", "application/json");
      xhr.onload = () => {
        var data = xhr.responseText;
        console.log("data: ", data);
        console.log("sent body: ", body);
      }
      xhr.send(body);
    }
  }

  belt_speed = document.getElementById("beltspeed");
  url = base_url + "/robot/set-speed";

  var xhr = new XMLHttpRequest();
  xhr.open("POST", url, true);
  xhr.setRequestHeader("Content-Type", "application/json");
  var body = JSON.stringify({ v:belt_speed.value });
  xhr.send(body);

}

function onclickSingleshot() {
  base_url = "http://" + document.getElementById("boxip").value + ":8080";
  var url = base_url + "/v2/camera/calibration/singleshot";
  var xhr = new XMLHttpRequest();
  xhr.open("POST", url, true);
  xhr.setRequestHeader("Content-Type", "application/json");

  var body = JSON.stringify({
    "trigger_single_shot" : true
  });

  xhr.onload = () => {
    var data = xhr.responseText;
    console.log("data: ", data);
  }
  console.log("body: ", body);
  xhr.send(body);
}

function updateSingleshot()
{
  base_url = "http://" + document.getElementById("boxip").value + ":8080";
  var url = base_url + "/v2/camera/calibration/singleshot";
  var xhr = new XMLHttpRequest();
  xhr.open("GET", url, true);

  xhr.onload = () => {
    var data = xhr.responseText;
    console.log("data: ", data);
    document.getElementById("singleshot_state_text").value = data;
  }
  xhr.send();
}


function saveButtonClicked()
{
  base_url = "http://" + document.getElementById("boxip").value + ":8080";
  var url = base_url + "/config_action";

  //{ "config": "save" }
  var body = "{\"config\": \"save\"}";

  var xhr = new XMLHttpRequest();
  xhr.open("POST", url, true);
  xhr.setRequestHeader("Content-Type", "application/json");
  xhr.onload = () => {
    var data = xhr.responseText;
    console.log("data: ", data);
    if(xhr.status == 200) {
      document.getElementById("save").style.backgroundColor = "green";
      setTimeout(function() {
        document.getElementById("save").style.backgroundColor = "";
      }, 500);
    }
    else
    {
      document.getElementById("save").style.backgroundColor = "red";
      setTimeout(function() {
        document.getElementById("save").style.backgroundColor = "";
      }, 500);
    }
  }
  xhr.send(body);
}

$('.dropdown-menu').on('click', function (e) {
  e.stopPropagation();
});

document.getElementById("dropdownMenuButton").addEventListener("click", function () {
  //If global_class_names is empty update it from the server
  if (global_class_names) {
    base_url = "http://" + document.getElementById("boxip").value + ":8080";
    //get class names
    url = base_url + "/v2/taxonomy/classes"
    xhr_class_names = new XMLHttpRequest();
    xhr_class_names.open("GET", url, true);
    xhr_class_names.onload = () => {
      var data = JSON.parse(xhr_class_names.response);
      global_class_names = data["taxonomy_list"];
    }
    xhr_class_names.send();
  }

    //Populate the dropdown
  //Clear the dropdown
  var dropdown = document.getElementById("class_lists");
  while (dropdown.firstChild) {
    dropdown.removeChild(dropdown.firstChild);
  }

  if (global_class_names) {
    for (var i = 0; i < global_class_names.length; i++) {
      var checkbox = document.createElement("input");
      checkbox.type = "checkbox";
      checkbox.value = global_class_names[i];
      checkbox.id = "class_checkbox_" + i;
      var label = document.createElement("label");
      label.htmlFor = "class_checkbox_" + i;
      label.innerHTML = global_class_names[i];
      document.getElementById("class_lists").appendChild(checkbox);
      document.getElementById("class_lists").appendChild(label);
            // add a new line
      var newLine = document.createElement("br");
      document.getElementById("class_lists").appendChild(newLine);
    }
  }
});

document.getElementById("Apply-Config").addEventListener("click", function () {
  // Check which items are checked
  var checkboxes = document.querySelectorAll("#class_lists input[type='checkbox']");
  var checkedItems = [];
  for (var i = 0; i < checkboxes.length; i++) {
    if (checkboxes[i].checked) {
      checkedItems.push(checkboxes[i].value);
    }
  }
  //console.log("Checked items: ", checkedItems);

  //build json object
  //{ "exclude": [ "graphic_paper" ], "s": "BIG", "turn-on-when-moving": 1, "history-filter": true }
  // check historyfilter-toggle state include for true, exclude for false
  var filter_type = document.getElementById("filter-toggle").checked ? "include" : "exclude";
  var enable_history_filter = document.getElementById("historyfilter-toggle").checked ? true : false;
  var json_body = { [filter_type]: checkedItems, s: "BIG", "turn-on-when-moving": 1, "history-filter": enable_history_filter };

  console.log("json_body: ", json_body);

  url = base_url + "/robot/config";
  var xhr = new XMLHttpRequest();
  xhr.open("POST", url, true);
  xhr.setRequestHeader("Content-Type", "application/json");
  var body = JSON.stringify(json_body);
  xhr.send(body);
  xhr.onload = () => {
    var data = xhr.responseText;
    console.log("response: ", data);
  }
});
