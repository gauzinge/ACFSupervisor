function SelectFile(val, Id){
    var element=document.getElementById('configfile'+Id.toString());
    //console.log(val, Id)
    if(val=='other')
      element.style.display='inline';
    else  
      element.style.display='none';
}


function SelectDefaultOption(id, val)
{    
    //console.log(id, val)
    var element = document.getElementById(id);
    if(element != null)
    {
        for(var i, j = 0; i = element.options[j]; j++) {
            if(i.value == val) {
                element.selectedIndex = j;
                break;
             }
        }
    }
}

function SetEventType(val) {
    //<!--console.log(val)-->
    <!--Get all options within <select id='foo'>...</select>-->
    var op = document.getElementById('eventType').getElementsByTagName('option');
    <!--if the value of boardType is not D19C, disable ZS -->
    if(val != 'D19C') {
        for(var i = 0; i < op.length; i++) {
            if(op[i].value=='ZS'){ op[i].disabled = true;
            //<!--console.log(op[i])-->
            }
        }
    }
    <!--if the value is D19C-->
    else {
        for (var i = 0; i < op.length; i++) {
            if(op[i].value == 'ZS') op[i].disabled = false;
            else op[i].disabled = false;
            //<!--console.log(op[i])-->
         }
    }
}

function DisplayFields(val, position){
    var Register = document.getElementById('conddata_Register_'+position.toString());
    var RegisterLabel = document.getElementById('conddata_RegisterLabel_'+position.toString());
    var FeId = document.getElementById('conddata_FeId_'+position.toString());
    var FeIdLabel = document.getElementById('conddata_FeIdLabel_'+position.toString());
    var CbcId = document.getElementById('conddata_CbcId_'+position.toString());
    var CbcIdLabel = document.getElementById('conddata_CbcIdLabel_'+position.toString());
    var UID = document.getElementById('conddata_UID_'+position.toString());
    var UIDLabel = document.getElementById('conddata_UIDLabel_'+position.toString());
    var Sensor = document.getElementById('conddata_Sensor_'+position.toString());
    var SensorLabel = document.getElementById('conddata_SensorLabel_'+position.toString());
    var Value = document.getElementById('conddata_Value_'+position.toString());
    var ValueLabel = document.getElementById('conddata_ValueLabel_'+position.toString());

    Register.style.display='none';
    RegisterLabel.style.display='none';
    FeId.style.display='none';
    FeIdLabel.style.display='none';
    CbcId.style.display='none';
    CbcIdLabel.style.display='none';
    UID.style.display='none';
    UIDLabel.style.display='none';
    Sensor.style.display='none';
    SensorLabel.style.display='none';
    Value.style.display='none';
    ValueLabel.style.display='none';

    if(val=="I2C")
    {
        Register.style.display='inline';
        RegisterLabel.style.display='inline';
        FeId.style.display='inline';
        FeIdLabel.style.display='inline';
        CbcId.style.display='inline';
        CbcIdLabel.style.display='inline';
    }
    else if(val=="User")
    {
        UID.style.display='inline';
        UIDLabel.style.display='inline';
        FeId.style.display='inline';
        FeIdLabel.style.display='inline';
        CbcId.style.display='inline';
        CbcIdLabel.style.display='inline';
        Value.style.display='inline';
        ValueLabel.style.display='inline';

    }
    else if(val=="HV")
    {
        FeId.style.display='inline';
        FeIdLabel.style.display='inline';
        Sensor.style.display='inline';
        SensorLabel.style.display='inline';
        Value.style.display='inline';
        ValueLabel.style.display='inline';
    }
    else if(val=="TDC")
    {
        FeId.style.display='inline';
        FeIdLabel.style.display='inline';
    }
    else
    {
        Register.style.display='none';
        RegisterLabel.style.display='none';
        FeId.style.display='none';
        FeIdLabel.style.display='none';
        CbcId.style.display='none';
        CbcIdLabel.style.display='none';
        UID.style.display='none';
        UIDLabel.style.display='none';
        Sensor.style.display='none';
        SensorLabel.style.display='none';
        Value.style.display='none';
        ValueLabel.style.display='none';
    }
}

function DisplayFieldsOnload(){
    <!--handle the condition data fields-->
    var RegEx = /^conddata_\d$/;
    var ConditionDataIds = [];
    var els=document.querySelectorAll('*[id^="conddata_"]');
    for(var i=0; i < els.length;i++){
        if(RegEx.test(els[i].id)) {
            ConditionDataIds.push(els[i].id);
        }
    }

    for(var i=0; i < ConditionDataIds.length;i++) {
        var elem = document.getElementById(ConditionDataIds[i]);
        if(elem != null) {
            var selected_node = elem.options[elem.selectedIndex].value;
            DisplayFields(selected_node, i+1);
        }
    }
    //<!--handle the event type input-->
    var boardtype = document.getElementById('boardType');
    if(boardtype != null) {
       var boardtypenode = boardtype.options[boardtype.selectedIndex].value;
       SetEventType(boardtypenode);
    }
}
        
function addConditionData(ul_name) {
    var ul =document.getElementById(ul_name);
    var counter = ul.getElementsByTagName("li").length;
    console.log(counter)
    var newEntry = document.createElement('li');
    newEntry.innerHTML = "Type:<select name='ConditionData_"+counter+"' id='conddata_"+counter+"' onchange='DisplayFields(this.value, "+counter+");'><option name=''></option><option name='I2C'>I2C</option><option name='User'>User</option><option name='HV'>HV</option><option name='TDC'>TDC</option></select><label for='conddata_Register_"+counter+"' id='conddata_RegisterLabel_"+counter+"' style='display:none'>Register:</label><input type='text' name='ConditionData_Register_"+counter+"' id='conddata_Register_"+counter+"' value='' size='10' style='display:none'/><label for='conddata_UID_"+counter+"' id='conddata_UIDLabel_"+counter+"' style='display:none'>UID:</label><input type='text' name='ConditionData_UID_"+counter+"' id='conddata_UID_"+counter+"' value='' size='5' style='display:none'/><label for='conddata_FeId_"+counter+"' id='conddata_FeIdLabel_"+counter+"' style='display:none'>FeId:</label><input type='text' name='ConditionData_FeId_"+counter+"' id='conddata_FeId_"+counter+"' value='' size='5' style='display:none'/><label for='conddata_CbcId_"+counter+"' id='conddata_CbcIdLabel_"+counter+"' style='display:none'>CbcId:</label><input type='text' name='ConditionData_CbcId_"+counter+"' id='conddata_CbcId_"+counter+"' value='' size='5' style='display:none'/> <label for='conddata_Sensor_"+counter+"' id='conddata_SensorLabel_"+counter+"' style='display:none'>Sensor:</label><input type='text' name='ConditionData_Sensor_"+counter+"' id='conddata_Sensor_"+counter+"' value='' size='5' style='display:none'/><label for='conddata_Value_"+counter+"' id='conddata_ValueLabel_"+counter+"' style='display:none'>Value:</label><input type='text' name='ConditionData_Value_"+counter+"' id='conddata_Value_"+counter+"' value='' size='5' style='display:none'/>";
ul.appendChild(newEntry);
}

window.onload = DisplayFieldsOnload();
