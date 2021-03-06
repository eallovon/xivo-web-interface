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

$form = &$this->get_module('form');
$url = &$this->get_module('url');

$info = $this->get_var('info');
$error = $this->get_var('error');
$element = $this->get_var('element');

$context_list = $this->get_var('context_list');
$ipbxinfos = $this->get_var('info','ipbx');

$allow = array();
if(isset($info['protocol'])):
	if (isset($info['protocol']['allow']))
		$allow =  $info['protocol']['allow'];
	$protocol = (string) dwho_ak('protocol',$info['protocol'],true);
	$context = (string) dwho_ak('context',$info['protocol'],true);
	$amaflags = (string) dwho_ak('amaflags',$info['protocol'],true);
	$qualify = (string) dwho_ak('qualify',$info['protocol'],true);
	$host = (string) dwho_ak('host',$info['protocol'],true);
else:
	$protocol = $this->get_var('proto');
	$context = $amaflags = $qualify = $host = '';
endif;

if ($protocol === '')
	$protocol = $info['linefeatures']['protocol'];

if ($this->get_var('act') == 'edit')
	echo $form->hidden(array('name' => 'protocol[context]','value' => $context));


$codec_active = empty($allow) === false;
$host_static = ($host !== '' && $host !== 'dynamic');

echo $form->hidden(array('name' => 'proto','value' => $protocol));

$filename = dirname(__FILE__).'/protocol/'.$protocol.'.php';
if (is_readable($filename) === true):
	include($filename);
endif;
?>

<div id="sb-part-ipbxinfos" class="b-nodisplay">
<div class="sb-list">
<table>
	<thead>
	<tr class="sb-top">
		<th class="th-left"><?=$this->bbf('col_line-key');?></th>
		<th class="th-right"><?=$this->bbf('col_line-value');?></th>
	</tr>
	</thead>
<?php
$i = 0;
if($ipbxinfos !== false
&& ($nb = count($ipbxinfos)) !== 0):
	foreach($ipbxinfos as $info_key => $info_value):
?>
	<tbody>
	<tr onmouseover="this.tmp = this.className; this.className = 'sb-content l-infos-over';"
	    onmouseout="this.className = this.tmp;"
	    class="fm-paragraph l-infos-<?=(($i++ % 2) + 1)?>on2">
		<td class="td-left"><?=$info_key?></td>
		<td class="td-right"><?=$info_value?></td>
	</tr>
<?php
	endforeach;
else:
?>
	<tfoot>
	<tr<?=($ipbxinfos !== false ? ' class="b-nodisplay"' : '')?>>
		<td colspan="2" class="td-single"><?=$this->bbf('no_ipbxinfos_found');?></td>
	</tr>
	</tfoot>
<?php
endif;
?>
	</tbody>
</table>
</div>
</div>
