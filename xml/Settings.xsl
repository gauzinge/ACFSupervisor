<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:output method="html" indent="yes" />

    <xsl:template match="/">
        <!--<table>-->
        <xsl:apply-templates match="Setting"/>
            <tr>
                <td>
                    <input type="text" name="setting" value="" size="30"/>
                </td>
                <td>
                    <input type="text" name="setting_value" value="" size="10"/>
                </td>
            </tr>
        <!--</table>-->
    </xsl:template>

    <xsl:template match="Setting">
            <tr>
                <td>
                    <input type="text" name="setting" value="{@name}" size="30"/>
                </td>
                <td>
                    <input type="text" name="setting_value" value="{.}" size="10"/>
                </td>
            </tr>
    </xsl:template>
    <xsl:template match="text()"/>
</xsl:stylesheet>
