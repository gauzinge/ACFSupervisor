<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:output method="xml" indent="yes" />
    <xsl:template match="/">
<HwDescription>
    <BeBoard Id="{//*[@name='beboard_id_0']/@value}" boardType="{//*[@name='boardType']/@value}" eventType="{//*[@name='eventType']/@value}">
        <connection id="{//*[@name='connection_id']/@value}" uri="{//*[@name='connection_uri']/@value}" address_table="{//*[@name='address_table']/@value}" />
    <Module FeId="" FMCId="" ModuleId="" Status="">

    </Module>
    </BeBoard>

</HwDescription>

<!--<xsl:template match="//*[@name=boardType]">-->
</xsl:template>
</xsl:stylesheet>
