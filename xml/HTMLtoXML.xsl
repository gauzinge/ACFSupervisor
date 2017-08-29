<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="xml" indent="yes" />

<xsl:template match="/">
    <HwDescription>
        <xsl:apply-templates name="BeBoard"/>
    </HwDescription>
</xsl:template>

<xsl:template match="//div[@class='BeBoard']" name="BeBoard">
    <BeBoard Id="{.//input[contains(@name,'beboard_id')]/@value}" boardType="{.//select[@name='boardType']//@value}" eventType="{.//select[@name='eventType']//@value}">
        <connection id="{.//input[@name='connection_id']/@value}" uri="{.//input[@name='connection_uri']/@value}" address_table="{.//input[@name='address_table']/@value}" />
        <!--now the module-->
        <xsl:apply-templates name="Module"/>
    </BeBoard>
</xsl:template>

<xsl:template match="//div[@class='Module']" name="Module">
    <Module FeId="{.//input[contains(@name,'fe_id')]/@value}" FMCId="{.//input[contains(@name,'fmc_id')]/@value}" ModuleId="{.//input[contains(@name,'module_id')]/@value}" Status="{.//input[contains(@name,'module_status')]/@value}">
        <xsl:call-template name="Global"/>
        <xsl:apply-templates select="//input[@name='cbc_filepath']"/>
        <xsl:apply-templates select="//select[contains(@name,'configfile')]"/>
    </Module>
</xsl:template>

<xsl:template match="//div[@class='Config']" name="Global">
    <Global>
        <Settings threshold="{//input[contains(@name,'Global__threshold')]/@value}" latency="{//input[contains(@name,'Global__latency')]/@value}"/>
        <TestPulse enable="{//input[contains(@name,'Global__tp_enable')]/@value}" polarity="{//select[contains(@name,'Global__tp_polarity')]//@value}" amplitude="{//input[contains(@name,'Global__tp_amplitude')]/@value}" channelgroup="{//input[contains(@name,'Global__tp_channelgroup')]/@value}" delay="{//input[contains(@name,'Global__tp_delay')]/@value}" groundothers="{//input[contains(@name,'Global__tp_groundothers')]/@value}"/>
       <ClusterStub clusterwidth="{//input[contains(@name,'Global__cs_clusterwidth')]/@value}" ptwidth="{//input[contains(@name,'Global__cs_ptwidth')]/@value}" layerswap="{//input[contains(@name,'Global__cs_layerswap')]/@value}" off1="{//input[contains(@name,'Global__cs_off1')]/@value}" off2="{//input[contains(@name,'Global__cs_off2')]/@value}" off3="{//input[contains(@name,'Global__cs_off3')]/@value}" off4="{//input[contains(@name,'Global__cs_off4')]/@value}"/>
       <Misc analogmux="{//input[contains(@name,'Global__misc_amux')]/@value}" pieplogic="{//input[contains(@name,'Global__misc_pipelogic')]/@value}" stublogic="{//input[contains(@name,'Global__misc_stublogic')]/@value}" or254="{//input[contains(@name,'Global__misc_or254')]/@value}" tpgclock="{//input[contains(@name,'Global__misc_tpgclock')]/@value}" testclock="{//input[contains(@name,'Global__misc_testclock')]/@value}" dll="{//input[contains(@name,'Global__misc_dll')]/@value}"/>
       <ChannelMask disable="{//input[contains(@name,'Global__chanmas_disable')]/@value}"/>
   </Global>
   <xsl:apply-templates select="//input[contains(@name,'glob_cbc_reg:')]"/>
</xsl:template>

<xsl:template match="//div[@class='CBCConfig']/ul" name="CBC_Settings">
    <!--<xsl:if test=". != ''">-->
        <Settings threshold="{//input[starts-with(@name,'CBC_') and contains(@name,'_threshold')]/@value}" latency="{//input[starts-with(@name,'CBC_') and contains(@name,'_latency')]/@value}"/>
        <TestPulse enable="{//input[starts-with(@name,'CBC_') and contains(@name,'_tp_enable')]/@value}" polarity="{//select[starts-with(@name,'CBC_') and contains(@name,'_tp_polarity')]//@value}" amplitude="{//input[starts-with(@name,'CBC_') and contains(@name,'_tp_amplitude')]/@value}" channelgroup="{//input[starts-with(@name,'CBC_') and contains(@name,'_tp_channelgroup')]/@value}" delay="{//input[starts-with(@name,'CBC_') and contains(@name,'_tp_delay')]/@value}" groundothers="{//input[starts-with(@name,'CBC_') and contains(@name,'_tp_groundothers')]/@value}"/>
       <ClusterStub clusterwidth="{//input[starts-with(@name,'CBC_') and contains(@name,'_cs_clusterwidth')]/@value}" ptwidth="{//input[starts-with(@name,'CBC_') and contains(@name,'_cs_ptwidth')]/@value}" layerswap="{//input[starts-with(@name,'CBC_') and contains(@name,'_cs_layerswap')]/@value}" off1="{//input[starts-with(@name,'CBC_') and contains(@name,'_cs_off1')]/@value}" off2="{//input[starts-with(@name,'CBC_') and contains(@name,'_cs_off2')]/@value}" off3="{//input[starts-with(@name,'CBC_') and contains(@name,'_cs_off3')]/@value}" off4="{//input[starts-with(@name,'CBC_') and contains(@name,'_cs_off4')]/@value}"/>
       <Misc analogmux="{//input[starts-with(@name,'CBC_') and contains(@name,'_misc_amux')]/@value}" pieplogic="{//input[starts-with(@name,'CBC_') and contains(@name,'_misc_pipelogic')]/@value}" stublogic="{//input[starts-with(@name,'CBC_') and contains(@name,'_misc_stublogic')]/@value}" or254="{//input[starts-with(@name,'CBC_') and contains(@name,'_misc_or254')]/@value}" tpgclock="{//input[starts-with(@name,'CBC_') and contains(@name,'_misc_tpgclock')]/@value}" testclock="{//input[starts-with(@name,'CBC_') and contains(@name,'_misc_testclock')]/@value}" dll="{//input[starts-with(@name,'CBC_') and contains(@name,'_misc_dll')]/@value}"/>
       <ChannelMask disable="{//input[starts-with(@name,'CBC_') and contains(@name,'_chanmas_disable')]/@value}"/>
    <!--</xsl:if>-->
   <!--<xsl:apply-templates select="//input[contains(@name,'glob_cbc_reg:')]"/>-->
</xsl:template>

<xsl:template match="//div[@class='CBCConfig']//input[starts-with(@name,'...')]" name="CBC_Register">
    <Register name="{substring(@name,5)}"><xsl:value-of select="@value"/></Register>

</xsl:template>

<xsl:template match="//input[@name='cbc_filepath']" name="CbcFilePath">
    <CBC_Files path="{./@value}" />
</xsl:template>

<xsl:template match="//input[contains(@name,'glob_cbc_reg:')]" name="Global_CBC_Register">
    <Global_CBC_Register name="{substring-after(@name,'glob_cbc_reg:')}"><xsl:value-of select="@value"/></Global_CBC_Register>
</xsl:template>

<xsl:template match="//select[contains(@name,'configfile')]" name="CBC">
    <CBC Id="{substring-after(@name,'configfile')}" configfile="{./option/@value}">
        <!--<xsl:value-of select="../../../."/>-->
        <!--<xsl:if test="../../../../div[@class='CBCConfig']/ul/li/input[contains(@name,'threshold')]">-->
            <xsl:call-template name="CBC_Settings"/>
            <!--<xsl:apply-templates match="//div[@class='CBCConfig']/ul/li"/>-->
        <!--</xsl:if>-->
    <xsl:apply-templates select="//div[@class='CBCConfig']//input[starts-with(@name,'...')]"/>
    </CBC>
</xsl:template>


<xsl:template match="text()"/>
</xsl:stylesheet>
