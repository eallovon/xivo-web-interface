<?php

#
# XiVO Web-Interface
# Copyright (C) 2006-2011  Avencall
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

include(dwho_file::joinpath(dirname(__FILE__),'..','_common.php'));

if(defined('XIVO_LOC_UI_ACTION') === true)
	$act = XIVO_LOC_UI_ACTION;
else
	$act = $_QRY->get('act');

/*
 *

$_QRY->get('context')
eg. default, intern ......

$_QRY->get('obj')
eg. user, group ...

$_QRY->get('startnum')
eg. 80, 422 ...

$_QRY->get('except')
eg.

 *
 */

switch($act)
{
	case 'search':
	default:
		$act = 'search';
		$appcontext = &$ipbx->get_application('context');
		$context = $_QRY->get('context');

		if(($context = $appcontext->get($context)) === false)
		{
			$http_response->set_status_line(404);
			$http_response->send(true);
		}

		$contextnummember = $context['contextnummember'];
		$contextnumbers = $context['contextnumbers'];

		$obj = $_QRY->get('obj');
		if(is_null($obj) || !array_key_exists($obj, $contextnumbers))
		{
			$http_response->set_status_line(204);
			$http_response->send(true);
		}

		$contextnumbers_obj = $contextnumbers[$obj];
		$contextnummember_obj = $contextnummember[$obj];

		$has_getnumpull = (bool) $_QRY->get('getnumpool');

		$filter = 0;
		if (is_null($_QRY->get('startnum')) === false)
			$filter  = $_QRY->get('startnum');
		elseif(is_null($_QRY->get('q')) === false)
			$filter  = $_QRY->get('q');

		if ($filter)
		{
			if(strlen($filter) > 0 && !is_numeric($filter))
			{
				$_TPL->set_var('list', array());
				break;
			}

			$filter  = intval($filter);
			$lfilter = floor(log10($filter)) + 1;
		}

		$numbers = array();
		$list_pool_free = array();
		foreach($contextnumbers_obj as $numb)
		{
			$start = intval($numb['numberbeg']);
			$end = intval($numb['numberend']);
			$lstart = floor(log10($start)) + 1;
			$lend = floor(log10($end)) + 1;

			if($has_getnumpull)
			{
				array_push($list_pool_free, array('numberbeg' => $start, 'numberend' => $end));
				continue;
			}

			if ($filter)
			{
				if($lfilter > $lend)
					continue;

				$fstart = intval($start / pow(10, $lstart - $lfilter));
				$fend = intval($end / pow(10, $lend - $lfilter));
				if($filter < $fstart || $fend < $filter)
					continue;

				$start = max($start, $filter * (pow(10, $lstart - $lfilter)));
				$end = min($end, ($filter+1) * (pow(10, $lend - $lfilter)) - 1);
			}

			$numbers = array_merge($numbers, range($start, $end));
		}

		if ($has_getnumpull)
		{
			$_TPL->set_var('list', $list_pool_free);
			$_TPL->display('genericjson');
		}

		$nb = count($contextnummember_obj);
		for($i=0; $i<$nb; $i++)
		{
			$ref = &$contextnummember_obj[$i];
			if(isset($numbers[$ref['number']]) === true)
				unset($numbers[$ref['number']]);
		}

		switch ($_QRY->get('format'))
		{
			case 'jquery':
				$list = '';
				foreach(array_values($numbers) as $num)
					$list .=  $num."\n";
				break;
			case null:
			default:
				$list = array();
				// just to respect suggest.js data format
				foreach(array_values($numbers) as $num)
					$list[] = array('id' => $num, 'identity' => strval($num), 'info' => '');
		}

		$_TPL->set_var('list', $list);
		break;
}

$_TPL->display('/service/ipbx/'.$ipbx->get_name().'/pbx_settings/extension');

?>
