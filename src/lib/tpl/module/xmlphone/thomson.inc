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

require_once(dwho_file::joinpath(DWHO_PATH_ROOT,'tpl','module','xmlphone','abstract','abstract.inc'));

class dwho_tpl_xmlphone_thomson extends dwho_tpl_xmlphone_abstract
{
    var $_vendor        = 'thomson';
    var $_model        = null;
    var $_argseparator    = '&amp;';
    var $_cdata        = true;
    var $_escape        = false;
    var $_utf8        = false;
    var $_nbbypage        = 8;
    var $_tags        = array('directory'    => 'ThomsonPhoneBook',
                    'menu'        => 'ThomsonPhoneMenu',
                    'input'        => null);

    function dwho_tpl_xmlphone_thomson()
    {
        $this->_model = $this->get_model();
    }

    function get_model()
    {
        if(isset($_SERVER['HTTP_USER_AGENT']) === false
        || preg_match('/^THOMSON (ST2022|ST2030|TB30) /',
                  $_SERVER['HTTP_USER_AGENT'],
                  $match) !== 1)
            return(null);

        return($match[1]);
    }
}

?>
