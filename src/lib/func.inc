<?php

#
# XiVO Web-Interface
# Copyright (C) 2006-2015  Avencall
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


function dwho_pagination_from_array($display_array, $start, $offset) {
	return array_slice($display_array, $start, $offset);
}

function dwho_load_extra_i18n($_I18N)
{
	$i18n_directory = XIVO_EXTRA.'/i18n.d'.'/'.DWHO_I18N_BABELFISH_LOCALE;
	$files = scandir($i18n_directory);
	foreach ($files as $file)
	{
		if (substr($file, -4) !== 'i18n')
			continue;
		$_I18N->load_file('extra/'.$file);
	}
}

function dwho_include_extra_files_from_dir($directory, &$extra_array=null)
{
	$files = scandir($directory);
	foreach ($files as $file)
	{
		if (substr($file, -3) !== 'inc')
			continue;
		require_once($directory.'/'.$file);
		if ($extra_array !== null)
			$extra_array = array_merge_recursive($array, $extra_array);
	}
}

function dwho_array_set_value_to_key($arr, $key_of_expected_value, $only_value=false)
{
	$arr = (array) $arr;
	$r = array();
	foreach($arr as $old_key => $element)
	{
		$key = $element[$key_of_expected_value];
		if ($only_value)
			$element = $key;
		$r[$key] = $element;
	}
	return($r);
}

function dwho_clean_cmd_list_to_process()
{
	$_SRE = dwho_gct::get('xivo_service');
	$_XOBJ = &dwho_gct::get('xivo_object');
	$ipbx = &$_SRE->get('ipbx');
	$ctimain = &$ipbx->get_module('ctimain');
	$request_handlers = &$_XOBJ->get_module('request_handlers');

	if (isset($_SESSION['cmd_list_to_process']) === false
	|| empty($_SESSION['cmd_list_to_process']) === true
	|| ($cti_data = $ctimain->get(1)) === false
	|| (bool) $cti_data['live_reload_conf'] === false)
	{
		$_SESSION['cmd_list_to_process'] = array();
		return(false);
	}

	$request_handlers->exec_cmd($_SESSION['cmd_list_to_process']);
	$_SESSION['cmd_list_to_process'] = array();
}

function dwho_error_log($msg, $status=null)
{
	if ($status !== 200)
		error_log(dwho_json::encode($msg));
}

function dwho_logw($str,$login,$act)
{
	if (defined('XIVO_LOG_FILE') === false)
		die('xivo_log_file_not_exist');

	if (empty($login) === true)
		$login = 'unknow';

	if (XIVO_LOG_TYPE === 'json')
		$type = 'web_services';
	elseif (XIVO_LOG_TYPE === 'www')
	$type = 'www';
	else
		$type = 'unknow';

	$datetime = date('c');
	$log = '['.$datetime.'] ['.$_SERVER['REMOTE_ADDR'].'] ['.$login.'] ['.$act.'] ['.$type.'] '.$str."\n";

	if(($fh = fopen(XIVO_LOG_FILE, 'a+')) !== false)
	{
		fputs($fh, $log, strlen($log));
		fclose($fh);
		return(true);
	}
	else
		die('ERR: can\'t create or write '.XIVO_LOG_FILE);
}

function dwho_generate_code($length=8)
{
	$password = "";
	$possible = "2346789bcdfghjkmnpqrtvwxyzBCDFGHJKLMNPQRTVWXYZ";
	$maxlength = strlen($possible);

	if ($length > $maxlength)
		$length = $maxlength;

	$i = 0;
	while ($i < $length)
	{
		$char = substr($possible, mt_rand(0, $maxlength-1), 1);
		if (!strstr($password, $char))
		{
			$password .= $char;
			$i++;
		}
	}

	return($password);
}


function dwho_constant($name,$default=null)
{
	if(defined($name) === true)
		return(constant($name));

	return($default);
}

function dwho_second_to($sec, $round=null)
{
	$sec = floatval($sec);

	$arr = array('s' => pow(10, 0), # second
			'ms' => pow(10, -3), # millisecond
			'µs' => pow(10, -6), # microsecond
			'ns' => pow(10, -9), # nanosecond
			'ps' => pow(10, -12) # picosecond
	);

	$arr_keys = array_keys($arr);

	for($c = 0;$sec <= 1 && $c < 5;$c++)
		$sec *= 1000;

	if (is_null($round) === false
			&& is_numeric($round) === true)
		$sec = round($sec, $round);
	return($sec.$arr_keys[$c]);

}

function dwho_get_memory($value,$name=true)
{
	$get_name = strtolower($value);

	if((bool) $name === true
			&& ($get_name === 'memory_limit'
					|| $get_name === 'post_max_size'
					|| $get_name === 'upload_max_filesize') === true)
		$mem = ini_get($get_name);
	else
		$mem = strtoupper($value);

	$arr = array('K' => 1024, 'M' => 1048576);

	$type = substr($mem,-1,1);

	if(dwho_digit($mem) !== false)
		$r = $mem;
	else if(isset($arr[$type]) === true)
		$r = substr($mem,0,strlen($mem)-1) * $arr[$type];
	else
		$r = false;

	return($r);
}

function dwho_size_iec($size)
{
	$size = floatval($size);

	if($size < 0)
		$size = sprintf('%u',$size);

	$arr = array('Bytes',
			'KiB',
			'MiB',
			'GiB',
			'TiB',
			'PiB',
			'EiB',
			'ZiB',
			'YiB');

	for($c = 0;$size >= 1024 && $c < 8;$c++)
		$size /= 1024;

	return(array(strval($size),$arr[$c]));
}

function dwho_size_si_to_byte($from,$size)
{
	$from = strtoupper(strval($from));
	$size = floatval($size);

	if($size < 0)
		$size = sprintf('%u',$size);

	$arr = array('B',
			'KB',
			'MB',
			'GB',
			'TB',
			'PB',
			'EB',
			'ZB',
			'YB');

	if(($pos = array_search($from,$arr,true)) === false)
		return(false);

	return(strval($size *= pow(1000,$pos)));
}

function dwho_bool($bool)
{
	$bool = strtolower(strval($bool));

	switch($bool)
	{
		case '1':
		case 'y':
		case 't':
		case 'on':
		case 'yes':
		case 'true':
			return(true);
		case '0':
		case 'n':
		case 'f':
		case 'off':
		case 'no':
		case 'false':
			return(false);
	}

	return((bool) $bool);
}

function dwho_var_to_pg_bool($bool)
{
	$bool = dwho_bool($bool);

	switch($bool)
	{
		case true:
			return('true');
		default:
		case false:
			return('false');
	}
}

function dwho_empty($val,$var=true)
{
	if(empty($val) === false
			|| $val === '0'
			|| $val === 0
			|| ($var !== true && $val !== $var) === true)
		return(false);

	return(true);
}

function dwho_has_len($val,$key=null)
{
	if($key !== null
			&& is_array($val) === true
			&& isset($val[$key]) === true)
		$val = $val[$key];

	if(is_scalar($val) === false)
		return(false);

	$val = (string) $val;

	return(isset($val{0}));
}

function dwho_digit($val=0,$def=null,$t=false)
{
	if(ctype_digit(strval($val)) === true)
		return($val);
	else if($def === null)
		return(false);
	else if((bool) $t === false)
		return($def);

	return((ctype_digit(strval($def)) === true ? $def : false));
}

function dwho_xdigit($val=0,$def=null,$t=false)
{
	if(ctype_xdigit(strval($val)) === true)
		return($val);
	else if($def === null)
		return(false);
	else if((bool) $t === false)
		return($def);

	$def = strval($def);

	return((ctype_xdigit(strval($def)) === true ? $def : false));
}

function dwho_uint($val,$def=0,$t=false)
{
	if(is_scalar($val) === false)
		$val = 0;
	else
	{
		$val = intval($val);

		if($val < 0)
			$val = (int) substr($val,1);
	}

	if($def !== 0 && $val === 0)
		$val = (bool) $t === false ? $def : dwho_uint($def);

	return($val);
}

function dwho_ufloat($val,$def=0,$t=false)
{
	if(is_scalar($val) === false)
		$val = 0;
	else
	{
		$val = sprintf('%.16F',$val);

		if(isset($val{0}) === true && $val{0} === '-')
			$val = substr($val,1);

		$val = floatval($val);
	}

	if($def !== 0 && (string) $val === '0')
		$val = (bool) $t === false ? $def : dwho_ufloat($def);

	if(is_scalar($val) === true && (string) $val === '0')
		return(0);

	return($val);
}

function dwho_ulongint($val,$def=0,$t=false)
{
	if(is_float($val) === false)
	{
		$val = strval($val);

		if(preg_match('/^[1-9][0-9]{0,19}$/',$val) === 1)
			return($val);
	}
	else if(($fl = sprintf('%.16F',$val)) === sprintf('%.16F',floor($val)))
	{
		$val = substr($fl,(int) ($fl{0} === '-'),-17);

		if($val !== '0')
			return($val);
	}

	$val = (bool) $t === false ? strval($def) : dwho_ulongint($def);

	if($val === '0')
		$val = 0;

	return($val);
}

function dwho_is_number($val)
{
	if(is_float($val) === true || is_int($val) === true)
		return(true);

	return((bool) preg_match('/^-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?$/',strval($val)));
}

function dwho_is_unumber($val)
{
	if(is_float($val) === true || is_int($val) === true)
		return(($val >= 0));

	return((bool) preg_match('/^(?:0|[1-9][0-9]*)(?:\.[0-9]+)?$/',strval($val)));
}

function dwho_unumber($val,$def=0,$t=false)
{
	if(is_float($val) === false)
		$val = strval($val);
	else
		$val = sprintf('%.16F',$val);

	if(preg_match('/^-?0*([0-9]+(?:\.[0-9]+)?)0*$/',$val,$match) === 1)
	{
		if(($val = rtrim($match[1],'.0')) === '')
			$val = 0;

		return($val);
	}

	$val = (bool) $t === false ? strval($def) : dwho_unumber($def);

	if($val === '0')
		$val = 0;

	return($val);
}

function dwho_sgraph($str='',$def=null,$t=false)
{
	if(dwho_is_sgraph($str) === true)
		return($str);
	else if($def === null)
		return(false);
	else if((bool) $t === false)
		return($def);

	return((dwho_is_sgraph($def) === true ? $def : false));
}

function dwho_is_digit($digit)
{
	if(is_scalar($digit) === false
			|| ctype_digit((string) $digit) === false)
		return(false);

	return(true);
}

function dwho_is_xdigit($xdigit)
{
	if(is_scalar($xdigit) === false
			|| ctype_xdigit((string) $xdigit) === false)
		return(false);

	return(true);
}

function dwho_is_int($int)
{
	return(((string) intval($int) === strval($int)));
}

function dwho_is_uint($uint)
{
	return((dwho_is_int($uint) === true && $uint >= 0));
}

function dwho_is_sgraph($str,$type='')
{
	if(is_scalar($str) === false)
		return(false);

	$type = strtolower($type);

	$reg = '\x00-\x08\x0B\x0C\x0E-\x1F\x7F';

	switch($type)
	{
		case 'tab':
			// All characters printable with TAB SPACE
			$reg .= '\x0A\x0D';
			break;
		case 'crlf':
			// All characters printable with CR LF SPACE
			$reg .= '\x09';
			break;
		case 'crlftab':
			// All characters printable with CR LF TAB SPACE
			break;
		default:
			// All characters printable with SPACE
			$reg .= '\x09\x0A\x0D';
	}

	if(preg_match('/['.$reg.']/u',$str) === 0)
		return(true);

	return(false);
}

function dwho_is_sgraph_crlf($str)
{
	return(dwho_is_sgraph($str,'crlf'));
}

function dwho_is_sgraph_tab($str)
{
	return(dwho_is_sgraph($str,'tab'));
}

function dwho_is_sgraph_crlf_tab($str,$type='')
{
	return(dwho_is_sgraph($str,'crlftab'));
}

function dwho_is_spunct($str,$type='',$space=true)
{
	$space = (bool) $space;

	if(is_scalar($str) === false)
		return(false);

	$type = strtolower($type);

	if($space === true)
		$reg = '\x20-';
	else
		$reg = '\x21-';

	$reg .= '\x2F\x3A-\x40\x5B-\x60\x7B-\x7E';

	switch($type)
	{
		case 'num':
			$reg .= '0-9';
			break;
		case 'alpha':
			$reg .= 'a-zA-Z';
			break;
		case 'alphanum':
			if($space === true)
				return(ctype_print($str));
			else
				$reg .= '0-9a-zA-Z';
			break;
		default:
			if($space === false)
				return(ctype_punct($str));
	}

	if(preg_match('/^['.$reg.']+$/u',$str) === 1)
		return(true);

	return(false);
}

function dwho_is_numspunct($str)
{
	return(dwho_is_spunct($str,'num'));
}

function dwho_is_alspunct($str)
{
	return(dwho_is_spunct($str,'alpha'));
}

function dwho_is_alnumspunct($str)
{
	return(dwho_is_spunct($str,'alphanum'));
}

function dwho_is_punct($str)
{
	return(dwho_is_spunct($str,null,false));
}

function dwho_is_numpunct($str)
{
	return(dwho_is_spunct($str,'num',false));
}

function dwho_is_alpunct($str)
{
	return(dwho_is_spunct($str,'alpha',false));
}

function dwho_is_alnumpunct($str)
{
	return(dwho_is_spunct($str,'alphanum',false));
}

function dwho_is_instance($obj,$name)
{
	if(is_object($obj) === false
			|| is_string($name) === false
			|| get_class($obj) !== $name)
		return(false);

	return(true);
}

function dwho_trunc(&$str,$nb=0,$end='',$chr=null,$trim=false)
{
	$r = strval($str);
	$nb = (int) $nb;
	$end = (string) $end;
	$chr = $chr === null ? ' ' : (string) $chr;

	if($nb > 0 && mb_strlen($r) > $nb && ($sub = mb_substr($r,0,$nb)) !== '')
	{
		$r = $sub;

		if($chr !== '' && ($spos = mb_strrpos($sub,$chr)) !== false)
			$r = mb_substr($r,0,$spos);

		if((bool) $trim === true)
			$r = trim($r);

		if(isset($end{0}) === true)
			$r .= $end;
	}
	else if((bool) $trim === true)
		$r = trim($r);

	return($r);
}

/**
 * \brief Checks that $a[$k] exists, $a being an array and $k being the key
 *
 * There are two options :
 * If $t is true, the return is the value of $a[$k]
 * If $t is false, the return is true if the key exists else false.
 *
 * If $f is true, existence is checked with isset.
 * If $f is false, existence is checked with array_key_exists.
 *
 * Return is false if any type check (array and key) fails.
 */
function dwho_ak($k,&$a,$t=false,$f=false)
{
	if(is_scalar($k) === false || is_array($a) === false)
		return(false);
	else if((bool) $f === false)
	{
		if(isset($a[$k]) === false)
			return(false);

		return(((bool) $t === false ? true : $a[$k]));
	}
	else if(array_key_exists($k,$a) === true)
		return(((bool) $t === false ? true : $a[$k]));

	return(false);
}

function dwho_pk($k,&$o,$t=false,$f=false)
{
	if(is_scalar($k) === false || is_object($o) === false)
		return(false);
	else if((bool) $f === false)
	{
		if(isset($o->{$k}) === false)
			return(false);

		return(((bool) $t === false ? true : $o->{$k}));
	}
	else if(array_key_exists($k,$o) === true)
		return(((bool) $t === false ? true : $o->{$k}));

	return(false);
}

function dwho_issa($k,&$a,$t=false)
{
	if(is_scalar($k) === false || is_array($a) === false)
		return(false);
	else if(isset($a[$k]) === true && is_array($a[$k]) === true)
		return(((bool) $t === false ? true : $a[$k]));

	return(false);
}

function dwho_issa_val($k,&$a,$e=false)
{
	if(dwho_issa($k,$a) === false)
		return(false);

	$r = array_values($a[$k]);

	if((bool) $e === false && array_key_exists(0,$r) === false)
		return(false);

	return($r);
}

function dwho_isso($k,&$o,$t=false)
{
	if(is_scalar($k) === false || is_object($o) === false)
		return(false);
	else if(isset($o->{$k}) === true && is_object($o->{$k}) === true)
		return(((bool) $t === false ? true : $o->{$k}));

	return(false);
}

function dwho_clone($obj)
{
	return(unserialize(serialize($obj)));
}

function dwho_group_array($ref,&$arr)
{
	if(dwho_issa($ref,$arr) === false
			|| ($nb = count($arr[$ref])) === 0
			|| is_array($arr) === false
			|| empty($arr) === true)
		return(false);

	$r = array();

	foreach($arr as $key => $val)
	{
		if(is_array($val) === false)
			continue;

		for($i = 0;$i < $nb;$i++)
		{
			if(isset($r[$i]) === false)
				$r[$i] = array();

			$r[$i][$key] = isset($val[$i]) === false ? '' : $val[$i];
		}
	}

	return($r);
}

function dwho_array_combine($keys,$values,$notexists=false,$defvalue=null)
{
	if(is_array($keys) === false || is_array($values) === false)
		return(false);

	$notexists = (bool) $notexists;

	$r = array();

	foreach($keys as $k => $v)
	{
		if(is_scalar($v) === false)
			continue;
		else if(array_key_exists($k,$values) === true)
			$r[$v] = $values[$k];
		else if($notexists === true)
			$r[$v] = $defvalue;
	}

	return($r);
}

function dwho_array_merge_replace_recursive()
{
	if(($argc = func_num_args()) === 0)
		return(null);
	else if($argc === 1)
		return((array) func_get_arg(0));

	$r = (array) func_get_arg(0);

	for($i = 1;$i < $argc;$i++)
	{
		$array = (array) func_get_arg($i);

		foreach($array as $key => $value)
		{
			if(is_numeric($key) === true
					&& in_array($value,$r) === false)
			{
				if(is_array($value) === true)
					$r[] = dwho_array_merge_replace_recursive($array[$key],$value);
				else
					$r[] = $value;
			}
			else if(isset($r[$key]) === true
					&& is_array($r[$key]) === true
					&& is_array($value) === true)
				$r[$key] = dwho_array_merge_replace_recursive($r[$key],$value);
			else
				$r[$key] = $value;
		}
	}

	return($r);
}

function dwho_array_startswith_key($arr,$str,$sub=false)
{
	$sub = (bool) $sub;

	if(is_array($arr) === false || dwho_has_len($str) === false)
		return(false);

	$len = strlen($str);

	$r = array();

	foreach($arr as $key => $value)
	{
		if(strncmp($str,$key,$len) === 0)
		{
			if($sub === true)
				$r[substr($key,$len)] = $value;
			else
				$r[$key] = $value;
		}
	}

	return($r);
}

function dwho_array_intersect_key($arr,$data,$key=null,$empty=false)
{
	$empty = (bool) $empty;

	$r = $empty === false ? false : array();

	if(is_array($arr) === false
			|| is_array($data) === false
			|| empty($arr) === true)
		return($r);

	$r = array();

	foreach($arr as $k => $v)
	{
		if($key === null)
		{
			if(isset($data[$k]) === true)
				$r[$k] = $data[$k];
		}
		else if(is_array($v) === true
				&& isset($v[$key]) === true
				&& is_scalar($v[$key]) === true
				&& isset($data[$v[$key]]) === true)
			$r[$v[$key]] = $data[$v[$key]];
	}

	if($empty === false && empty($r) === true)
		$r = false;

	return($r);
}

function dwho_array_copy_intersect_key($arr,$data,$key=null,$empty=false)
{
	$empty = (bool) $empty;

	$r = $empty === false ? false : array();

	if(is_array($arr) === false
			|| is_array($data) === false
			|| empty($arr) === true)
		return($r);

	$r = array();

	foreach($arr as $k => $v)
	{
		if($key === null)
		{
			if(isset($data[$k]) === true)
				$r[$k] = $v;
		}
		else if(is_array($v) === true
				&& isset($v[$key]) === true
				&& is_scalar($v[$key]) === true
				&& isset($data[$v[$key]]) === true)
			$r[$v[$key]] = $v;
	}

	if($empty === false && empty($r) === true)
		$r = false;

	return($r);
}

function dwho_array_diff_key($arr1,$arr2)
{
	if(is_array($arr1) === false || is_array($arr2) === false)
		return(false);

	$r = array();

	$diff = array_values(array_diff(array_keys($arr1),array_keys($arr2)));

	if(($nb = count($diff)) === 0)
		return($r);

	for($i = 0;$i < $nb;$i++)
		$r[$diff[$i]] = $arr1[$diff[$i]];

	return($r);
}

function dwho_array_reduce_level($arr,$t=false)
{
	if(is_array($arr) === false)
		return(false);

	$r = array();

	$arr = array_values($arr);

	if(($nb = count($arr)) === 0)
		return($r);

	if((bool) $t === false)
	{
		for($i = 0;$i < $nb;$i++)
			$r = array_merge(array_keys($arr[$i]),$r);
	}
	else
	{
		for($i = 0;$i < $nb;$i++)
			$r = array_merge(array_keys(array_flip($arr[$i])),$r);
	}

	return($r);
}

function dwho_array_change_value_case($arr,$case=CASE_LOWER)
{
	return(array_flip(array_change_key_case(array_flip($arr),intval((bool) $case))));
}

function dwho_array_same_key_val($arr,$key)
{
	$r = array();

	foreach($arr as $k => $v)
	{
		$r[$k] = $v[$key];
	}

	if(empty($r) === true)
		$r = false;

	return($r);
}

function dwho_build_array_ref(&$a)
{
	if(is_array($a) === false)
		return(false);

	$arr = array_values($a);

	if(($nb = count($arr)) === 0)
		return(false);

	$cnt = $nb - 1;

	$r['array'] = $r['ref'] = array();
	$ref = &$r['array'];

	for($i = 0;$i < $cnt;$i++)
	{
		$ref[$arr[$i]] = array();
		$ref = &$ref[$arr[$i]];
	}

	$ref[$arr[$i]] = '';
	$r['ref'] = &$ref[$arr[$i]];

	return($r);
}

function dwho_unserialize(&$str)
{
	$r = array();

	if(is_string($str) === true && empty($str) === false)
		$r = unserialize($str);

	return($r);
}

function dwho_print_r($a,$n='')
{
	if(dwho_empty($n) === false)
		print '<b>'.$n.'</b> : ';
	print '<pre>'; print_r($a); print '</pre>';
}

function dwho_var_dump_html($var,$title='')
{
	echo '<pre style="border:1px dotted gray; padding:5px; text-align:left;">';
	if (dwho_empty($title) === false)
		echo '<h4>', htmlspecialchars($title), '</h4>';

	ob_start();
	var_dump($var);
	echo htmlspecialchars(preg_replace("/\]\=\>\n(\s+)/m", '] => ', ob_get_clean()));
	echo '</pre>';
}

function dwho_var_dump($var,$title='')
{
	echo '<pre style="border:1px dotted gray; padding:5px; text-align:left; background:#ddd;">';
	if(dwho_empty($title) === false)
		print '<h4 style="border-bottom:1px solid black; background:#ccc;">'.$title.'</h4>';
	ob_start();
	var_dump($var);
	echo preg_replace("/\]\=\>\n(\s+)/m", '] => ', ob_get_clean());
	echo '</pre>';
}

function dwho_hr($msg='')
{
	echo '<pre style="border:1px solid black; padding:2px; background:black;">';
	if(dwho_empty($msg) === false)
		print '<h4 style="color:white; font:bold; text-align:center;">'.(string) $msg.'</h4>';
	echo '</pre>';
}

function dwho_sprint_r(&$a,$t=false)
{
	ob_start();
	print_r($a);
	return(((bool) $t === false ? ob_get_clean() : nl2br(ob_get_clean())));
}

function dwho_svar_dump(&$a,$t=false)
{
	ob_start();
	var_dump($a);
	return(((bool) $t === false ? ob_get_clean() : nl2br(ob_get_clean())));
}

function dwho_msg($msg='')
{
	if(dwho_has_len($msg) === true)
		$msg = ': '.$msg;
	else
		$msg = '';

	if(defined('DWHO_LABEL_CUSTOM') === true)
		$label = DWHO_LABEL_CUSTOM;
	else
		$label = DWHO_LABEL;

	return($label.$msg);
}

function dwho_die($msg='')
{
	die(dwho_msg($msg));
}

function dwho_eol($str)
{
	return(preg_replace('#\r\n|\n|\r#',DWHO_EOL,strval($str)));
}

function dwho_alttitle($str,$sc=true)
{
	$pat = array('/<.+?>/s','/\t+/s','/\s+/s','/&nbsp;| +/s');
	$rep = array('','',' ',' ');
	$r = preg_replace($pat,$rep,strval($str));

	return(((bool) $sc === true ? dwho_htmlsc($r) : $r));
}

function dwho_notag($str)
{
	$pat = array('/<.+?>/s','/\t+/s','/&nbsp;| +/s');
	$rep = array('','',' ');

	return(preg_replace($pat,$rep,strval($str)));
}

function dwho_notagscript($str)
{
	return(preg_replace('@<script[^>]*?>.*?</script[^>]*?>@si','',strval($str)));
}

function dwho_tagreplace($str)
{
	$pat = array('"','\'','<','>');
	$rep = array('&quot;','&#039;','&lt;','&gt;');

	return(str_replace($pat,$rep,strval($str)));
}

function dwho_htmlsc($str,$style=ENT_QUOTES)
{
	return(htmlspecialchars(strval($str),$style,'UTF-8'));
}

function dwho_htmlen($str,$style=ENT_QUOTES)
{
	return(htmlentities(strval($str),$style,'UTF-8'));
}

function dwho_last_key(&$a,$sort=SORT_NUMERIC)
{
	if(is_array($a) === false)
		return(false);

	$ak = array_keys($a);
	ksort($ak,$sort);

	return(end($ak));
}

function dwho_his_to_second($his)
{
	$arr = split('[:/]',$his);
	$time = 0;
	$time += ($arr[0] * 3600);
	$time += ($arr[1] * 60);
	$time += ($arr[2]);

	return($time);
}

function dwho_sec_to_time($second)
{

	$second = (int) $second;
	$sec = dwho_calc_duration('','',$second);

	$res = '';

	if($sec['d'] != 0
			|| $sec['h'] != 0)
	{
		$h = ($sec['h']+($sec['d']*24));
		if (strlen($h) === 1)
			$h = '0'.$h;
		$res .= $h.':';
	}
	else
		$res .= '00:';

	if($sec['m'] != 0)
	{
		$m = ($sec['m']);
		if (strlen($m) === 1)
			$m = '0'.$m;
		$res .= $m.':';
	}
	else
		$res .= '00:';

	if($sec['s'] != 0)
	{
		$s = round($sec['s'],0);
		if (strlen($s) === 1)
			$s = '0'.$s;
		$res .= $s;
	}
	else
		$res .= '00';

	if($sec['h'] == 0
			&& $sec['m'] == 0
			&& $sec['s'] == 0)
		$res = '00:00:00';

	return($res);
}

function dwho_sec_to_human($sec,$d=true,$h=true,$m=true,$s=false) {

	$sec = (int) $sec;
	$sec = dwho_calc_duration('','',$sec);

	$res = '';
	if($d === true
			&& $sec['d'] != 0)
		$res .= $sec['d'].'j ';
	elseif(((bool) $d) === false
			&& $sec['d'] != 0)
	{
		$sec['h'] = $sec['d']*24;
	}

	if($h === true
			&& $sec['h'] != 0)
		$res .= $sec['h'].'h ';

	if($m === true
			&& $sec['m'] != 0)
		$res .= $sec['m'].'m ';

	if($s === true
			&& $sec['s'] != 0)
		$res .= round($sec['s'],0).'s';

	if($sec['d'] == 0
			&& $sec['h'] == 0
			&& $sec['m'] == 0
			&& $sec['s'] == 0)
		$res = 0;

	return($res);
}

function dwho_calc_time($from,$time,$format,$valzero=true)
{
	$valzero = (bool) $valzero;

	if(is_numeric($time) === false)
		return(false);

	switch($from)
	{
		case 'day':
			$time *= 24;
		case 'hour':
			$time *= 60;
		case 'minute':
			$time *= 60;
		case 'second':
			$time *= 1000;
		case 'millisecond':
			$time *= 1000;
		case 'microsecond':
			break;
		default:
			return(false);
	}

	$r = array();

	if(strpos($format,'%d') !== false)
	{
		if(($r['d'] = floor($time / 86400000000)) === (float) 0
				&& $valzero === false)
			unset($r['d']);
		else
			$time -= $r['d'] * 86400000000;
	}

	if(strpos($format,'%H') !== false)
	{
		if(($r['H'] = floor($time / 3600000000)) === (float) 0
				&& $valzero === false)
			unset($r['H']);
		else
			$time -= $r['H'] * 3600000000;
	}

	if(strpos($format,'%M') !== false)
	{
		if(($r['M'] = floor($time / 60000000)) === (float) 0
				&& $valzero === false)
			unset($r['M']);
		else
			$time -= $r['M'] * 60000000;
	}

	if(strpos($format,'%s') !== false)
	{
		if(($r['s'] = floor($time / 1000000)) === (float) 0
				&& $valzero === false)
			unset($r['s']);
		else
			$time -= $r['s'] * 1000000;
	}

	if(strpos($format,'%ms') !== false)
	{
		if(($r['ms'] = floor($time / 1000)) === (float) 0
				&& $valzero === false)
			unset($r['ms']);
		else
			$time -= $r['ms'] * 1000;
	}

	if(strpos($format,'%f') !== false
			&& ($r['f'] = (int) $time) === 0
			&& $valzero === false)
		unset($r['f']);

	return($r);
}

function dwho_calc_duration($beg,$end,$diff=false,$unset=false)
{
	$r = array();

	if(is_numeric($diff) === true)
		$r['diff'] = $diff;
	else if(is_numeric($beg) === false
			|| is_numeric($end) === false)
		return(false);
	else
	{
		$r['beg'] = $beg;
		$r['end'] = $end;
		$r['diff'] = $end - $beg;
	}

	$r['s']  = $r['diff'];
	$r['d']  = floor($r['s'] / 86400);
	$r['s'] -= $r['d'] * 86400;
	$r['h']  = floor($r['s'] / 3600);
	$r['s'] -= $r['h'] * 3600;
	$r['m']  = floor($r['s'] / 60);
	$r['s'] -= $r['m'] * 60;

	ksort($r);

	if((bool) $unset === false)
		return($r);

	unset($r['beg'],$r['end'],$r['diff']);

	if($r['d'] === (float) 0)
		unset($r['d']);
	else
		return($r);

	if($r['h'] === (float) 0)
		unset($r['h']);
	else
		return($r);

	if($r['m'] === (float) 0)
		unset($r['m']);

	return($r);
}

function dwho_date_parse_from_format($stFormat, $stData)
{
	$aDataRet			 = array();
	$aDataRet['day']	 = 0;
	$aDataRet['month']	 = 0;
	$aDataRet['year']	 = 0;
	$aDataRet['hour']	 = 0;
	$aDataRet['minute'] = 0;
	$aDataRet['second'] = 0;
	$aPieces = split('[:/.\ \-]', $stFormat);
	$aDatePart = split('[:/.\ \-]', $stData);
	foreach($aPieces as $key => $chPiece)
	{
		if (isset($aDatePart[$key]) === false)
			continue;
		switch ($chPiece)
		{
			case 'd':
			case 'j':
				$aDataRet['day'] = (int) $aDatePart[$key];
				break;
			case 'F':
			case 'M':
			case 'm':
			case 'n':
				$aDataRet['month'] = (int) $aDatePart[$key];
				break;
			case 'o':
			case 'Y':
			case 'y':
				$aDataRet['year'] = (int) $aDatePart[$key];
				break;
			case 'g':
			case 'G':
			case 'h':
			case 'H':
				$aDataRet['hour'] = (int) $aDatePart[$key];
				break;
			case 'i':
				$aDataRet['minute'] = (int) $aDatePart[$key];
				break;
			case 's':
				$aDataRet['second'] = (int) $aDatePart[$key];
				break;
		}
	}
	return($aDataRet);
}

function dwho_change_date_format($stDate,$stFormatFrom,$stFormatTo)
{
	$date = dwho_date_parse_from_format($stFormatFrom,$stDate);
	return(date($stFormatTo,mktime($date['hour'],
			$date['minute'],
			$date['second'],
			$date['month'],
			$date['day'],
			$date['year'])));
}

function dwho_calc_page($page,$nb,$total)
{
	$r = array(
			'page'	=> 1,
			'nb'	=> 0,
			'pages'	=> 0,
			'total' => 0,
			'next'	=> 0,
			'prev'	=> 0,
			'beg'	=> 0,
			'end'	=> 0);

	$r['page'] = dwho_uint($page,1);
	$r['nb'] = dwho_uint($nb,10);
	$r['total'] = dwho_uint($total);

	if($r['total'] === 0)
		return($r);

	$r['pages'] = ceil($r['total']/$r['nb']);

	if($r['page'] > $r['pages'])
		$r['page'] = 1;

	$r['end'] = $r['page'] * $r['nb'];
	$r['beg'] = $r['end'] - $r['nb'];

	if($r['end'] < $r['total'])
		$r['next'] = true;

	if($r['page'] > 1 && $r['total'] > $r['nb'])
		$r['prev'] = true;

	return($r);
}



function dwho_gm2localtime($format,$time)
{
	$time = intval($time);

	return(strftime($format,$time + date('Z',$time)));
}

function dwho_printf_escape($format,$argc=0)
{
	$argc = dwho_uint($argc);

	$rs = preg_split('/(%+)/',$format,-1,PREG_SPLIT_DELIM_CAPTURE);

	if(($nb = count($rs)) === 1)
		return($rs[0]);

	$r = '';

	for($i = 0,$j = 1,$k = 0;$i < $nb;$i += 2,$j += 2)
	{
		$r .= $rs[$i];

		if($j >= $nb)
			break;
		else if((strlen($rs[$j]) % 2) === 0 || $argc > $k++)
			$r .= $rs[$j];
		else
			$r .= '%'.$rs[$j];
	}

	return($r);
}

function dwho_vksprintf($format,$args)
{
	if(is_array($args) === false)
		return(false);

	$rs = preg_split('/(%+\{([a-zA-Z][a-zA-Z0-9_]*)\}\$'.
			'(?:\+\-)?(?: 0|\'.)?(?:\-?[0-9]*(?:\.[0-9]+)?)?'.
			'[bcdeufFosxX])/',
			$format,
			-1,
			PREG_SPLIT_DELIM_CAPTURE);

	if(($nb = count($rs)) === 1)
		return(vsprintf(dwho_printf_escape($format,count($args)),$args));

	$vsfargs = $args;

	$r = '';

	for($i = 0,$j = 1,$k = 2;$i < $nb;$i += 3,$j += 3,$k += 3)
	{
		$r .= $rs[$i];

		if($j >= $nb)
			break;

		$pos = strpos($rs[$j],'{');

		if(($pos % 2) === 0)
			$r .= $rs[$j];
		else if(isset($args[$rs[$k]]) === false)
			$r .= '%'.$rs[$j];
		else
		{
			$f = '%'.substr($rs[$j],strlen($rs[$k]) + $pos + 3);
			$r .= str_replace('%','%%',sprintf($f,strval($args[$rs[$k]])));
			unset($vsfargs[$rs[$k]]);
		}
	}

	return(vsprintf(dwho_printf_escape($r,count($vsfargs)),$vsfargs));
}

function dwho_replacef($search,$replace,$subject)
{
	if(is_scalar($subject) === false)
		return(false);
	else if(dwho_has_len($search) === false)
		return($subject);
	else if(strpos($search,'%') !== false)
		return(false);

	$split = preg_split('/(%+)/',$subject,-1,(PREG_SPLIT_NO_EMPTY|PREG_SPLIT_DELIM_CAPTURE));

	if(($nb = count($split)) === 0)
		return($subject);

	$r = '';

	$len = strlen($search);

	for($i = 0,$j = 1;$i < $nb;$i++,$j++)
	{
		if($split[$i]{
			0} === '%'
			&& isset($split[$j]) === true
			&& substr($split[$j],0,$len) === $search
			&& (strlen($split[$i]) % 2) === 1)
				$r .= substr($split[$i++],1).$replace.substr($split[$j++],$len);
			else
				$r .= $split[$i];
	}

	return($r);
}

function dwho_implode($glue,$pieces)
{
	if(is_scalar($glue) === false)
		return(false);
	else if(is_scalar($pieces) === true)
		return((string) $pieces);
	else if(is_array($pieces) === false && is_object($pieces) === false)
		return(false);

	return(implode((string) $glue,
			array_filter((array) $pieces,'is_scalar')));
}

function dwho_get_limit($offset,$line=null)
{
	if(is_array($offset) === true && isset($offset[0]) === true)
	{
		if(isset($offset[1]) === true)
			$line = $offset[1];

		$offset = $offset[0];
	}

	if(dwho_has_len($offset) === false)
		return(false);

	$r = array(dwho_uint($offset));

	if(dwho_has_len($line) === true)
		$r[1] = dwho_uint($line);

	return($r);
}

function dwho_is_max_digit($min,$max,$equal=false)
{
	if(dwho_digit($min) === false || dwho_digit($max) === false)
		return(null);
	else if(strlen($min) === strlen($max))
		return((bool) $equal === false ? ($max > $min) : ($max >= $min));

	return(false);
}

function dwho_get_max_digit($digit1,$digit2)
{
	if(dwho_digit($digit1) === false || dwho_digit($digit2) === false)
		return(false);
	else if(strlen($digit1) === strlen($digit2))
		return($digit1 > $digit2 ? $digit1 : $digit2);

	return($digit1);
}

function dwho_generate_uuid_v4()
{
	// stolen from https://gist.github.com/dahnielson/508447

	return sprintf('%04x%04x-%04x-%04x-%04x-%04x%04x%04x',

						// 32 bits for "time_low"
						mt_rand(0, 0xffff), mt_rand(0, 0xffff),

						// 16 bits for "time_mid"
						mt_rand(0, 0xffff),

						// 16 bits for "time_hi_and_version",
						// four most significant bits holds version number 4
						mt_rand(0, 0x0fff) | 0x4000,

						// 16 bits, 8 bits for "clk_seq_hi_res",
						// 8 bits for "clk_seq_low",
						// two most significant bits holds zero and one for variant DCE1.1
						mt_rand(0, 0x3fff) | 0x8000,

						// 48 bits for "node"
						mt_rand(0, 0xffff), mt_rand(0, 0xffff), mt_rand(0, 0xffff)
		);
}

?>
