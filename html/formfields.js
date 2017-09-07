
function displayACFfields() {
    var calibration = document.getElementById("Calibration");
    var pedenoise = document.getElementById("Pedestal&Noise");
    var commission = document.getElementById("Commissioning");

    if(calibration.checked || pedenoise.checked || commission.checked) {
        if(commission.checked) {
            //document.getElementById("commission_fieldset").style.display="inline";
            document.getElementById("commission_legend").style.display="inline";
            document.getElementById("commission_table").style.display="inline";
        }
        else {
            //document.getElementById("commission_fieldset").style.display="none";
            document.getElementById("commission_legend").style.display="none";
            document.getElementById("commission_table").style.display="none";
        }
            document.getElementById("result_directory").style.display="inline";
            document.getElementById("result_directory_label").style.display="inline";
            document.getElementById("settings_fieldset").style.display="inline";
            document.getElementById("settings_fieldset_legend").style.display="inline";
            document.getElementById("settings_table").style.display="inline";
    }
    else {
            document.getElementById("result_directory").style.display="none";
            document.getElementById("result_directory_label").style.display="none";
            document.getElementById("settings_fieldset").style.display="none";
            document.getElementById("settings_fieldset_legend").style.display="none";
            document.getElementById("settings_table").style.display="none";
            document.getElementById("commission_table").style.display="none";
    }
}

function queryState(){
    var state = document.getElementById("state").innerHTML;
    return state;
}

function addSetting(tableName) {
    var table = document.getElementById(tableName);
    var counter = table.getElementsByTagName("tbody")[0].getElementsByTagName("tr").length;
    console.log(counter)
    var newRow = document.createElement('tr');
    newRow.innerHTML = "<td><input type='text' name='setting_"+counter+"' value='' size='30'/></td> <td><input type='text' name='setting_value_"+counter+"' value='' size='10'/></td>";
    console.log(newRow)
    table.getElementsByTagName("tbody")[0].appendChild(newRow);
}

function submitACFForm() {
    var acfform = document.getElementById("acfform");
    console.log(acfform)
    acfform.submit();
}

function greyOutElements() {
    var HWDescriptionFile = document.getElementById("HwDescriptionFile");
    var HWDescriptionSubmit = document.getElementById("hwForm_load_submit");
    var acfForm = document.getElementById("acfform");
    var procedurelist = document.getElementById("procedurelist");
    var settingstable = document.getElementById("settings_table");
    var fileOptions_raw = document.getElementById("fileOptions_raw"); 
    var fileOptions_daq = document.getElementById("fileOptions_daq"); 
    var runnumber = document.getElementById("runnumber");
    var nevents = document.getElementById("nevents");
    var datadir = document.getElementById("datadir");
    var resultdir = document.getElementById("resultdir");
    var commission_table = document.getElementById("commission_table");
    var main_submit = document.getElementById("main_submit");

    var state = queryState();
    //first, grey out elements that should not be available once initialised
    if(state!='I') {
        HWDescriptionFile.setAttribute('disabled','disabled');
        HWDescriptionSubmit.setAttribute('disabled','disabled');
        runnumber.setAttribute('disabled','disabled');
        datadir.setAttribute('disabled','disabled');
        fileOptions_raw.setAttribute('disabled','disabled');
        fileOptions_daq.setAttribute('disabled','disabled');
    }
    else if(state == 'E') {
        main_submit.setAttribute('disabled','disabled');
        procedurelist.setAttribute('disabled','disabled');
        settingstable.setAttribute('disabled','disabled');
        nevents.setAttribute('disabled','disabled');
        resultdir.setAttribute('disabled','disabled');
        commission_table.setAttribute('disabled','disabled');
    }

    //console.log(state)

}

//https://stackoverflow.com/questions/18614301/keep-overflow-div-scrolled-to-bottom-unless-user-scrolls-up
function scrollLogToBottom() {
    var log = document.getElementById("logwindow");
    if(log) {
        //allow 2 px inaccuracy by adding 2 
        var isScrolledToBottom = log.scrollHeight - log.clientHeight <= log.scrollTop +2;
        if(isScrolledToBottom) log.scrollTop = log.scrollHeight - log.clientHeight;
    }
}

var log = document.getElementById("logwindow");
if(log) {
     log.scrollTop = log.scrollHeight - log.clientHeight;
}

var thing = setInterval(scrollLogToBottom, 1500);

//var statecheck = setInterval(greyOutElements, 2500);

function addLoadEvent(func) { 
  var oldonload = window.onload; 
  if (typeof window.onload != 'function') { 
    window.onload = func; 
  } else { 
    window.onload = function() { 
      if (oldonload) { 
        oldonload(); 
      } 
      func(); 
    } 
  } 
} 

addLoadEvent(displayACFfields); 
addLoadEvent(scrollLogToBottom);
addLoadEvent(queryState); 
//addLoadEvent(greyOutElements); 
