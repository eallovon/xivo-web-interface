<?php

#
# XiVO Web-Interface
# Copyright (C) 2010-2014  Avencall
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

$array = array();

$array['element'] = array();

$array['element']['destination'] = array();
$array['element']['destination']['default'] = '';

$array['element']['netmask'] = array();
$array['element']['netmask']['default'] = '';

$array['element']['calllimits'] = array();
$array['element']['calllimits']['default'] = '';

$array['filter'] = array();
$array['filter']['destination'] = array('callback'  => 'dwho_network::valid_host',
	'callbackarg' => array(DWHO_NETWORK_HOST_IPV4_DOTDEC));
$array['filter']['netmask'] = array('set'   => false,
	'callback'    => 'dwho_network::valid_mask_ipv4',
	'callbackarg' => array(DWHO_NETWORK_MASK_IPV4_DOTDEC));
$array['filter']['calllimits'] = array('set' => true, 'cast' => 'uint');

?>
