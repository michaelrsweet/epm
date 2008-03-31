<?php
//
// "$Id$"
//
// Global PHP constants and variables...
//
// This file should be included using "include_once"...
//

//
// Global vars...
//

$PROJECT_BUGS = "epm-bugs@easysw.com";	// Bug reporting address
$PROJECT_EMAIL = "epm@easysw.com";	// Default notification address
$PROJECT_MODULE = "epm";		// SVN module
$PROJECT_NAME = "ESP Package Manager";	// Title of project
$PROJECT_REGISTER = "epm-register@easysw.com";
					// User registration email
$PROJECT_RFES = "epm@easysw.com";	// Feaure request address
$PROJECT_SECURITY = "mike@easysw.com";	// Security reports address
$PROJECT_URL = "http://www.epmhome.org/";
					// URL to main site


//
// PHP transition stuff...
//

global $_COOKIE, $_FILES, $_GET, $_POST, $_SERVER;

foreach (array("argc", "argv", "REQUEST_METHOD", "SERVER_NAME", "SERVER_PORT", "REMOTE_ADDR") as $var)
{
  if (array_key_exists($var, $_SERVER))
    $$var = $_SERVER[$var];
  else
    $$var = "";
}

// Handle PHP_SELF differently - we need to quote it properly...
if (array_key_exists("PHP_SELF", $_SERVER))
  $PHP_SELF = htmlspecialchars($_SERVER["PHP_SELF"], ENT_QUOTES);
else
  $PHP_SELF = "";

if (array_key_exists("ISHTTPS", $_SERVER))
  $PHP_URL = "https://$SERVER_NAME:$SERVER_PORT$PHP_SELF";
else
  $PHP_URL = "http://$SERVER_NAME:$SERVER_PORT$PHP_SELF";

// Max items per page
if (array_key_exists("${PROJECT_MODULE}PAGEMAX", $_COOKIE))
{
  $PAGE_MAX = (int)$_COOKIE["${PROJECT_MODULE}PAGEMAX"];

  if ($PAGE_MAX < 10)
    $PAGE_MAX = 10;
}
else
  $PAGE_MAX = 10;

//
// End of "$Id$".
//
?>
