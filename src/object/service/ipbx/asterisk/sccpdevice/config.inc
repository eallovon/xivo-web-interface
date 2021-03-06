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

$array['element']['device'] = array();
$array['element']['device']['default'] = '';

$array['element']['line'] = array();
$array['element']['line']['default'] = '';

$array['element']['voicemail'] = array();
$array['element']['voicemail']['default'] = '';

dwho::load_class('dwho_network');

$array['filter'] = array();
$array['filter']['name'] = array('minlen' => 1,'maxlen' => 80,'regexp' => '/^[a-z0-9_-]+$/i');
$array['filter']['device'] = array('minlen' => 1,'maxlen' => 80,'regexp' => '/^[a-z0-9_- ]+$/i');
$array['filter']['line'] = array('minlen' => 1,'maxlen' => 80,'regexp' => '/^[a-z0-9_-]+$/i');
$array['filter']['voicemail'] = array('minlen' => 1,'maxlen' => 80,'regexp' => '/^[a-z0-9_-]+$/i');

?>
