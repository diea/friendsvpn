<?php
/**
 * My toolbox, contains small useful functions
 */

if ( ! defined('BASEPATH')) exit('No direct script access allowed');

if (!function_exists('ipVersion')) {
    function ipVersion($txt) {
        return strpos($txt, ":") === false ? 4 : 6;
    }
}
