<?php

#
# XiVO Web-Interface
# Copyright (C) 2006-2014  Avencall
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

class dwho_tpl_xmlphone_abstract
{
    var $_argseparator    = '&amp;';
    var $_cdata        = false;
    var $_escape    = true;
    var $_utf8        = true;
    var $_noaccents    = false;
    var $_nbbypage        = 16;
    var $_tags        = array('directory'    => null,
                    'menu'        => null,
                    'input'        => null);

    function has_cdata()
    {
        return($this->_cdata);
    }

    function has_escape()
    {
        return($this->_escape);
    }

    function has_utf8()
    {
        return($this->_utf8);
    }

    function arg_separator()
    {
        return($this->_argseparator);
    }

    function get_vendor()
    {
        return($this->_vendor);
    }

    function get_header_contenttype()
    {
        $charset = $this->has_utf8() === true ? 'utf-8' : 'iso-8859-15';

        return('Content-Type: text/xml; charset='.$charset);
    }

    function nb_by_page()
    {
        return($this->_nbbypage);
    }

    function escape($str,$force=true)
    {
        if(is_scalar($str) === false)
            return('');
        else if($this->has_cdata() === true)
            $str = '<![CDATA['.$str.']]>';
        else if($this->has_escape() === true || (bool) $force === true)
            $str = dwho_htmlsc($str,ENT_NOQUOTES);

        if($this->_noaccents)
        $str = str_replace(
            array('à','á','â','ã','ä', 'ç', 'è','é','ê','ë', 'ì','í','î','ï', 'ñ', 'ò','ó','ô','õ','ö', 'ù','ú','û','ü', 'ý','ÿ', 'À','Á','Â','Ã','Ä', 'Ç', 'È','É','Ê','Ë', 'Ì','Í','Î','Ï', 'Ñ', 'Ò','Ó','Ô','Õ','Ö', 'Ù','Ú','Û','Ü', 'Ý'), 
            array('a','a','a','a','a', 'c', 'e','e','e','e', 'i','i','i','i', 'n', 'o','o','o','o','o', 'u','u','u','u', 'y','y', 'A','A','A','A','A', 'C', 'E','E','E','E', 'I','I','I','I', 'N', 'O','O','O','O','O', 'U','U','U','U', 'Y'), 
            $str); 
            
        if($this->has_utf8() === false)
            $str = utf8_decode($str);

        return($str);
    }

    function has_tag_directory()
    {
        return(dwho_has_len($this->_tags['directory']));
    }

    function has_tag_menu()
    {
        return(dwho_has_len($this->_tags['menu']));
    }

    function has_tag_input()
    {
        return(dwho_has_len($this->_tags['input']));
    }

    function tag_directory()
    {
        return($this->_tags['directory']);
    }

    function tag_menu()
    {
        return($this->_tags['menu']);
    }

    function tag_input()
    {
        return($this->_tags['input']);
    }
}

?>
