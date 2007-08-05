#!/usr/bin/php -f
<?php
//
// Convert old ESP database to new database format...
//

$db = mysql_connect("localhost", "mike", "");

function
do_query($query, $db)
{
  $result = mysql_query($query, $db);
  if (!$result)
  {
    print("ERROR: " . mysql_error() . "\n$query\n");
    exit(1);
  }

  return ($result);
}

do_query("DELETE FROM epm.article", $db);

function
escape_values(&$row)
{
  reset($row);
  while (list($name, $value) = each($row))
    $row[$name] = mysql_escape_string(utf8_encode(trim($value)));
}

$progress = 99;

function
start_progress($title)
{
  global $progress;

  $progress = 99;
  print("$title: ");
  flush();
}

function
print_progress()
{
  global $progress;

  $progress = ($progress + 1) % 100;

  if ($progress == 0)
  {
    print(".");
    flush();
  }
}

function
first_paragraph($contents)
{
  $contents = trim($contents);
  $len      = strlen($contents);

  if (substr($contents, 0, 3) == "<p>" ||
      substr($contents, 0, 3) == "<P>")
  {
    if (($pos1 = strpos($contents, "</p>", 3)) === FALSE)
      if (($pos1 = strpos($contents, "</P>", 3)) === FALSE)
        $pos1 = strlen($contents);

    if (($pos2 = strpos($contents, "<p>", 3)) === FALSE)
      if (($pos2 = strpos($contents, "<P>", 3)) === FALSE)
        $pos2 = strlen($contents);

    if ($pos1 < $pos2)
      $paragraph = substr($contents, 3, $pos1 - 3);
    else
      $paragraph = substr($contents, 3, $pos2 - 3);
  }
  else
  {
    print("contents=\"$contents\"\n");

    $lines     = explode("\n", $contents);
    $paragraph = $lines[0];
  }

  // Strip HTML from paragraph...
  return (ereg_replace("<[^>]+>", "", $paragraph));
}


function
convert_faq($filename, $db)
{
  $fp       = fopen($filename, "r");
  $section  = "";
  $title    = "";
  $contents = "";
  $date     = time();

  while ($line = fgets($fp, 1024))
  {
    if (ereg("SECTION:.*", $line))
      $section = mysql_escape_string(trim(substr($line, 8)));
    else if (ereg("FAQ:.*", $line))
    {
      if ($title != "" && $contents != "")
      {
	$abstract = mysql_escape_string(first_paragraph($contents));
	$contents = mysql_escape_string($contents);

        do_query("INSERT INTO epm.article VALUES(NULL,1,\"$section\","
	        ."\"$title\",\"$abstract\",\"$contents\",$date,\"mike\","
		."$date,\"mike\")", $db);
      }

      $title    = mysql_escape_string(trim(substr($line, 4)));
      $contents = "";
    }
    else if (ereg("HowTo:.*", $line))
    {
      if ($title != "" && $contents != "")
      {
	$abstract = mysql_escape_string(first_paragraph($contents));
	$contents = mysql_escape_string($contents);

        do_query("INSERT INTO epm.article VALUES(NULL,1,\"$section\","
	        ."\"$title\",\"$abstract\",\"$contents\",$date,\"mike\","
		."$date,\"mike\")", $db);
      }

      $title    = mysql_escape_string(trim(substr($line, 6)));
      $contents = "";
    }
    else
      $contents .= $line;
  }

  if ($title != "" && $contents != "")
  {
    $abstract = mysql_escape_string(first_paragraph($contents));
    $contents = mysql_escape_string($contents);

    do_query("INSERT INTO epm.article VALUES(NULL,1,\"$section\","
	    ."\"$title\",\"$abstract\",\"$contents\",$date,\"mike\","
	    ."$date,\"mike\")", $db);
  }

  fclose($fp);
}

// FAQs...
convert_faq("faq.dat", $db);

?>
