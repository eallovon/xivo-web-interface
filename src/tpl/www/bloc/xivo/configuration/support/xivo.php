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

$form	 = &$this->get_module('form');
$dhtml 	 = &$this->get_module('dhtml');

$info = $this->get_var('info');
$element = $this->get_var('element');

?>

<div class="b-infos b-form">
<h3 class="sb-top xspan">
	<span class="span-left">&nbsp;</span>
	<span class="span-center"><?=$this->bbf('title_content_name');?></span>
	<span class="span-right">&nbsp;</span>
</h3>

<div class="sb-content">
<form action="#" method="post" accept-charset="utf-8">

<?php
	echo	$form->hidden(array('name' => DWHO_SESS_NAME, 'value' => DWHO_SESS_ID)),
			$form->hidden(array('name' => 'fm_send', 'value' => 1));
?>
	<div class="fm-paragraph fm-description">
		<p>
			<label id="lb-description" for="it-description"><?=$this->bbf('fm_xivo_uuid');?></label>
		</p>
<?php
	echo	$form->text(array('paragraph' => false, 'value' => $info['xivo_uuid'], 'size' => 50));
?>
		<p>
			<label id="lb-description" for="it-description"><?=$this->bbf('fm_support_key');?></label>
		</p>
<?php
	echo	$form->textarea(array('paragraph'	=> false,
					'label'	=> false,
					'name'		=> 'support_key',
					'id'		=> 'it-support_key',
					'cols'		=> 60,
					'rows'		=> 2),
					$info['support_key']);
?>
	</div>
<div class="sb-list" style="width: 350px; margin: auto;">
<table>
	<thead>
	<tr class="sb-top">
		<th class="th-single"><?=$this->bbf('col-expiration_date');?></th>
	</tr>
	</thead>
	<tbody>
	<tr class="fm-paragraph">
		<td class="td-single"><?=(!dwho_has_len($info['date_expiration'])) ? '-' : $info['date_expiration']?></td>
	</tr>
	</tbody>
</table>
<?php
echo $form->submit(array('name' => 'submit', 'id' => 'it-submit', 'value' => $this->bbf('fm_bt-save')));
?>
</div>
</form>
	</div>
	<div class="sb-foot xspan">
		<span class="span-left">&nbsp;</span>
		<span class="span-center">&nbsp;</span>
		<span class="span-right">&nbsp;</span>
	</div>
</div>
