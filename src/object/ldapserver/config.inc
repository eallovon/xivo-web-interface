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

$array = array();

$array['element'] = array();

$array['element']['name'] = array();
$array['element']['name']['default'] = '';

$array['element']['host'] = array();
$array['element']['host']['default'] = '';

$array['element']['port'] = array();
$array['element']['port']['default'] = 389;

$array['element']['securitylayer'] = array();
#$array['element']['securitylayer']['value'] = array('tls','ssl');
$array['element']['securitylayer']['value'] = array('ssl');
$array['element']['securitylayer']['default'] = '';
$array['element']['securitylayer']['null'] = true;

$array['element']['protocolversion'] = array();
$array['element']['protocolversion']['value'] = array('2','3');
$array['element']['protocolversion']['default'] = '3';
$array['element']['protocolversion']['setdef'] = true;

$array['element']['disable'] = array();
$array['element']['disable']['value'] = array(0,1);
$array['element']['disable']['default'] = 0;
$array['element']['disable']['set'] = true;

$array['element']['description'] = array();
$array['element']['description']['default'] = '';


dwho::load_class('dwho_network');

$array['filter'] = array();
$array['filter']['name'] = array('regexp' => '/^[a-z0-9_\.-]+$/','minlen' => 1,'maxlen' => 64);
$array['filter']['host'] = array('callback' => 'dwho_network::valid_host');
$array['filter']['port'] = array('cast' => 'uint','between' => array(1,65535));
$array['filter']['securitylayer'] = array('set' => false,'chk' => 2,'key' => array('tls','ssl'));
$array['filter']['protocolversion'] = array('key' => array('2','3'));
$array['filter']['disable'] = array('bool' => true);
$array['filter']['description'] = array('set' => false,'chk' => 2,'maxlen' => 1000,'cast' => 'sgraph_crlf_tab','eol' => true);

?>
