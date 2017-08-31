<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:output method="xml" indent="yes" />

    <xsl:template match="/">
        <Settings>
            <xsl:for-each select="div/tr">
                <xsl:call-template name="Setting"/>
            </xsl:for-each>
        </Settings>
    </xsl:template>

    <xsl:template match="tr" name="Setting">
        <Setting name="{td/input[@size='30']/@value}"><xsl:value-of select="td/input[@size='10']/@value"/></Setting>
    </xsl:template>
    <xsl:template match="text()"/>
</xsl:stylesheet>
