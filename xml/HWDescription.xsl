<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:output method="xml" indent="yes" version='1.0'/>

    <!--now parse the whole thing-->
    <xsl:template match="/*">
        <!--<html>-->
            <div>
            <body onload='DisplayFieldsOnload();'>
                <h3>HWDescription</h3>
                <xsl:apply-templates select="BeBoard"/>
            </body>
                <script type="text/javascript">
                 function SelectFile(val, Id){
                    var element=document.getElementById('configfile'+Id.toString());
                    console.log(val, Id)
                    if(val=='other')
                      element.style.display='inline';
                    else  
                      element.style.display='none';
                }


                function SelectDefaultOption(id, val)
                {    
                    console.log(id, val)
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
                    <!--console.log(val)-->
                    <!--Get all options within <select id='foo'>...</select>-->
                    var op = document.getElementById('eventType').getElementsByTagName('option');
                    <!--if the value of boardType is not D19C, disable ZS -->
                    if(val != 'D19C') {
                        for(var i = 0; i &lt; op.length; i++) {
                            if(op[i].value=='ZS'){ op[i].disabled = true;
                            <!--console.log(op[i])-->
                            }
                        }
                    }
                    <!--if the value is D19C-->
                    else {
                        for (var i = 0; i &lt; op.length; i++) {
                            if(op[i].value == 'ZS') op[i].disabled = false;
                            else op[i].disabled = false;
                            <!--console.log(op[i])-->
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
                    for(var i=0; i &lt; els.length;i++){
                        if(RegEx.test(els[i].id)) {
                            ConditionDataIds.push(els[i].id);
                        }
                    }

                    for(var i=0; i &lt; ConditionDataIds.length;i++) {
                        var elem = document.getElementById(ConditionDataIds[i]);
                        console.log(elem)
                        if(elem != null)
                        {
                            var selected_node = elem.options[elem.selectedIndex].value;
                            DisplayFields(selected_node, i+1);
                        }
                    }
                    <!--handle the event type input-->
                    var boardtype = document.getElementById('boardType');
                    if(boardtype != null) {
                        var boardtypenode = boardtype.options[boardtype.selectedIndex].value;
                        SetEventType(boardtypenode);
                    }

                }
                
                 window.onload = DisplayFieldsOnload();
                </script> 
            </div>
            <!--</html>-->
    </xsl:template>

    <!--Template for BeBoard nodes-->
    <xsl:template match="BeBoard">
        <div class="BeBoard">
            <h4> BeBoard Id: <input type="number" name="beboard_id_{@Id}" value="{@Id}"> </input></h4>
            <div class="General">
            <li>
            BoardType: 
                <select name="boardType" id="boardType" onchange="SetEventType(this.value);">
                    <option value="{@boardType}"><xsl:value-of select="@boardType"/></option>
                    <option value="D19C">D19C</option>
                    <option value="CBC3FC7">CBC3FC7</option>
                    <option value="GLIB">GLIB</option>
                    <option value="CTA">CTA</option>
                    <option value="ICGLIB">ICGLIB</option>
                    <option value="ICFC7">ICFC7</option>
                </select>
            </li>
            <li>
            EventType:
                    <select name="eventType" id="eventType">
                        <option value="{@eventType}"><xsl:value-of select="@eventType"/></option>
                    <option value="VR">VR</option>
                    <option value="ZS">ZS</option>
                    </select>
            </li>
            <li>
            Connection Id:
            <input type="text" name="connection_id" size="10" value="{connection/@id}"> </input>
            </li>
            <li>
            URI: 
            <input type="text" name="connection_uri" size="28" value="{connection/@uri}">
                </input>
            </li>
            <li>
            Address Table:
            <input type="text" name="address_table" size="48" value="{connection/@address_table}">
                </input>
            </li>
        </div>
        
            <!--Module-->
            <xsl:apply-templates select="Module"/>
            <!--SLINK-->
            <xsl:apply-templates select="SLink"/>

            <!--Registers-->
            <div class="BeBoardRegisters">
                <h4>Registers</h4>
                <ul>
                <xsl:apply-templates select="Register"/>
            </ul>
            </div>
        </div>
    </xsl:template>

    <!--Template for Module nodes-->
    <xsl:template match="Module">
        <div class="Module">
            <h4> Module</h4>
             <tr>
                 <td>  FeId: <input type="number" name="fe_id{@FeId}" size="2" value="{@FeId}"/></td>
                 <td>  FMCId: <input type="number" name="fmc_id{@FeId}" size="2" value="{@FMCId}" min="0" max="1"/></td>
                 <td>  ModuleId: <input type="number" name="module_id{@FeId}" size="2" value="{@ModuleId}"/></td>
                 <td>  Status: <input type="number" name="module_status{@FeId}" size="2" value="{@Status}" min="0" max="1"/></td>
             </tr>
            <div>
                CBC FilePath: <input type="text" name="cbc_filepath" size="20" value="{CBC_Files/@path}"/>
            <!--</input>-->
            <xsl:apply-templates select="Global"/>
            <ul>
            <xsl:apply-templates select="Global_CBC_Register"/>
            </ul>
            </div>
            <div>
            <ul>
                <xsl:apply-templates select="CBC"/>
            </ul>
            </div>
        </div>
    </xsl:template>

    <!--Template for CBC nodes-->
    <xsl:template match="CBC">
        <li>
            <tr>
                <td>CBC Id: <xsl:value-of select="@Id"/></td>
                <td>ConfigFile:</td> 
                <td>
            <select name="configfile{@Id}" onchange='SelectFile(this.value,{@Id});'>
                    <option value="{@configfile}"><xsl:value-of select="@configfile"/></option>
                    <option value="Cbc_default_electron.txt">Cbc_default_electron.txt</option>
                    <option value="Cbc_default_hole.txt">Cbc_default_hole.txt</option>
                    <option value="CBC3_default.txt">CBC3_default.txt</option>
                    <option value="Cbc_default_irrad.txt">Cbc_default_irrad.txt</option>
                    <option value="FE0CBC{@Id}">FE0CBC<xsl:value-of select="@Id"/></option>
                    <option value="other">other</option>
            </select>
            </td>
            <td>
                <input type="text" name="configfile{@Id}" id="configfile{@Id}" style="display:none;" placeholder="your filename"/>
            </td>
        </tr>
        </li>
        <div class="Config">
        <ul>
            <xsl:apply-templates select="Settings"/>
            <xsl:apply-templates select="TestPulse"/>
            <xsl:apply-templates select="ClusterStub"/>
            <xsl:apply-templates select="Misc"/>
            <xsl:apply-templates select="ChannelMask"/>
            <xsl:if test="Register">
            <li> CBCRegisters: </li>
            <ul>
                <xsl:apply-templates select="Register"/>
            </ul>
            </xsl:if>
        </ul>
        </div>
    </xsl:template>

    <!--Template for Global|CBC Settings node-->
    <xsl:template match="Settings">
        <li> Settings:  
            threshold = <input type="text" name="{name(..)}_{../@Id}_threshold" size="4" value="{@threshold}">
            </input>
        latency=<input type="text" name="{name(..)}_{../@Id}_latency" size="4" value="{@latency}">
            </input>
        </li>
    </xsl:template>

    <!--Template for Global|CBC TestPulse node-->
    <xsl:template match="TestPulse">
        <li> TestPulse: enable
            <input type="number" name="{name(..)}_{../@Id}_tp_enable" size ="1" min="0" max="1" value="{@enable}"/>
            polarity=
            <select name="{name(..)}_{../@Id}_tp_polarity" id="{name(..)}_{../@Id}_tp_polarity">
                <option value="{@polarity}"><xsl:value-of select="@polarity"/></option>
                <option value="0">electron</option>
                <option value="1">hole</option>
            </select>
            amplitude=
            <input type="text" name="{name(..)}_{../@Id}_tp_amplitude" size ="5" value="{@amplitude}">
           </input>
            channelgroup=
            <input type="number" name="{name(..)}_{../@Id}_tp_channelgroup" size="5" min="0" max="7" value="{@channelgroup}"/>
            delay=
            <input type="number" name="{name(..)}_{../@Id}_tp_delay" size="5" min="0" max="24" value="{@delay}"/>
            groundOthers=
            <input type="number" name="{name(..)}_{../@Id}_tp_groundothers" size ="1" min="0" max="1" value="{@groundothers}"/>
        </li> 
    </xsl:template>

    <!--Template for Global|CBC ClusterStub node-->
    <xsl:template match="ClusterStub">
        <li> ClusterStub: clusterwidth=
            <input type="text" name="{name(..)}_{../@Id}_cs_clusterwidth" size ="1" value="{@clusterwidth}">
            </input>
            ptwidth=
            <input type="text" name="{name(..)}_{../@Id}_cs_ptwidth" size ="1" value="{@ptwidth}">
           </input>
            layerswap=
                <input type="number" name="{name(..)}_{../@Id}_cs_layerswap" size ="1" min="0" max="1" value="{@layerswap}"/>
            off1=
            <input type="text" name="{name(..)}_{../@Id}_cs_off1" size ="2" value="{@off1}">
           </input>
            off2=
            <input type="text" name="{name(..)}_{../@Id}_cs_off2" size ="2" value="{@off2}">
           </input>
            off3=
            <input type="text" name="{name(..)}_{../@Id}_cs_off3" size ="2" value="{@off3}">
           </input>
            off4=
            <input type="text" name="{name(..)}_{../@Id}_cs_off4" size ="2" value="{@off4}">
           </input>
        </li> 
    </xsl:template>

    <!--Template for Global|CBC Misc node-->
    <xsl:template match="Misc">
        <li> Misc: analogmux=
            <input type="text" name="{name(..)}_{../@Id}_misc_amux" size ="8" value="{@analogmux}">
            </input>
            pipelogic=
            <input type="text" name="{name(..)}_{../@Id}_misc_piplogic" size ="1" value="{@pipelogic}">
           </input>
            stublogic=
            <input type="text" name="{name(..)}_{../@Id}_misc_stublogic" size ="1" value="{@stublogic}">
           </input>
            or254=
            <input type="number" name="{name(..)}_{../@Id}_misc_or254" size ="1" min="0" max="1" value="{@or254}"/>
            tpgclock=
            <input type="number" name="{name(..)}_{../@Id}_misc_tpgclock" size ="1" min="0" max="1" value="{@tpgclock}"/>
            testclock=
            <input type="number" name="{name(..)}_{../@Id}_misc_testclock" size ="1" min="0" max="1" value="{@testclock}"/>
            dll=
            <input type="text" name="{name(..)}_{../@Id}_misc_dll" size ="2" value="{@dll}">
           </input>
        </li> 
    </xsl:template>

    <!--Template for Global|CBC ChannelMask node-->
    <xsl:template match="ChannelMask">
        <li> ChannelMask (comma separated list):
            <input type="text" name="{name(..)}_{../@Id}_chanmas_disable" size ="18" value="{@disable}">
            </input>
        </li>
    </xsl:template>

    <!--Template for GlobalNodes-->
    <xsl:template match="Global">
        <div class="Config">
        <h5> Global CBC Config </h5>
            <ul>
                <xsl:apply-templates select="Settings"/>
                <xsl:apply-templates select="TestPulse"/>
                <xsl:apply-templates select="ClusterStub"/>
                <xsl:apply-templates select="Misc"/>
                <xsl:apply-templates select="ChannelMask"/>
            </ul>
        </div>
    </xsl:template>

    <!--Template for GlobalCBCRegister node-->
    <xsl:template match="Global_CBC_Register">
        <li> GlobalCBCRegister: <xsl:value-of select="@name"/>
            <input type="text" name="glob_cbc_reg:{@name}" size ="5" value="{.}"/>
            <!--</input>-->
        </li>
    </xsl:template>


    <!--Template for SLINK node-->
    <xsl:template match="SLink">
    <div class="SLink">
    <h4>SLink</h4>
        <ul>
            <xsl:apply-templates select="DebugMode"/>
            <xsl:apply-templates select="ConditionData"/>
        </ul>
    </div>
    </xsl:template>

    <!--Template for SLink Debug Mode node-->
    <xsl:template match="DebugMode">
        <li>
             Debug Mode:
            <select name="debugMode" id="debugMode">
                <option name="{@type}"><xsl:value-of select="@type"/></option>
                <option name="FULL">FULL</option>
                <option name="SUMMARY">SUMMARY</option>
                <option name="ERROR">ERROR</option>
            </select>
       </li>
   </xsl:template>

    <!--Template for SLINKConditionData node-->
    <xsl:template match="ConditionData">
               <li>
                   Type:
                   <select name="ConditionData_{position()}" id="conddata_{position()}" onchange='DisplayFields(this.value, {position()});'>
                        <option name="{@type}"><xsl:value-of select="@type"/></option>
                        <option name="I2C">I2C</option>
                        <option name="User">User</option>
                        <option name="HV">HV</option>
                        <option name="TDC">TDC</option>
                   </select>
                        
                   <label for="conddata_Register_{position()}" id="conddata_RegisterLabel_{position()}" style="display:none">Register:</label>
                   <input type="text" name="ConditionData_Register_{position()}" id="conddata_Register_{position()}" value="{@Register}" size="10" style="display:none"/>

                   <label for="conddata_UID_{position()}" id="conddata_UIDLabel_{position()}" style="display:none">UID:</label>
                   <input type="text" name="ConditionData_UID_{position()}" id="conddata_UID_{position()}" value="{@UID}" size="5" style="display:none"/>

                   <label for="conddata_FeId_{position()}" id="conddata_FeIdLabel_{position()}" style="display:none">FeId:</label>
                   <input type="text" name="ConditionData_FeId_{position()}" id="conddata_FeId_{position()}" value="{@FeId}" size="5" style="display:none"/>

                   <label for="conddata_CbcId_{position()}" id="conddata_CbcIdLabel_{position()}" style="display:none">CbcId:</label>
                   <input type="text" name="ConditionData_CbcId_{position()}" id="conddata_CbcId_{position()}" value="{@CbcId}" size="5" style="display:none"/> 
                   
                   <label for="conddata_Sensor_{position()}" id="conddata_SensorLabel_{position()}" style="display:none">Sensor:</label>
                   <input type="text" name="ConditionData_Sensor_{position()}" id="conddata_Sensor_{position()}" value="{@Sensor}" size="5" style="display:none"/>

                   <label for="conddata_Value_{position()}" id="conddata_ValueLabel_{position()}" style="display:none">Value:</label>
                   <input type="text" name="ConditionData_Value_{position()}" id="conddata_Value_{position()}" value="{.}" size="5" style="display:none"/>
               </li>

   </xsl:template>
    

    <!--Template for RegisterNode with children-->
    <xsl:template match="Register[Register]">
        <li><xsl:value-of select="@name"/></li>
            <ul>
                <xsl:apply-templates select="Register"/>
            </ul>
    </xsl:template>

    <!--Template for Register Node without children-->
    <xsl:template match="Register[not(Register)]">
        <li> <xsl:value-of select="@name"/>:
            <!--<xsl:value-of select="@name"/>-->
            <input type="text" name="{../../../@name}.{../../@name}.{../@name}.{../@Id}{@name}" size ="5" value="{.}">
                <!--<xsl:attribute name="Register">-->
                    <!--<xsl:value-of select="Register"/>-->
                <!--</xsl:attribute>-->
            </input>
        </li>
    </xsl:template>

</xsl:stylesheet>
