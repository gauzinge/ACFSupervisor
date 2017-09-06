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


function submitACFForm() {
    var acfform = document.getElementById("acfform");
    console.log(acfform)
    acfform.submit();
    //acfform.addEventListener("blur",acfform.submit());
    //document.forms["acfform"].submit();
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

var thing = setInterval(scrollLogToBottom, 1000);

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
//addLoadEvent(submitHWFile); 
//addLoadEvent(function() { 
