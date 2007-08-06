<?php
//
// "$Id$"
//
// Class for the article table.
//
// Contents:
//
//   article::article()		- Create an Article object.
//   article::clear()		- Initialize a new an Article object.
//   article::delete()		- Delete an Article object.
//   article::edit()		- Display a form for an Article object.
//   article::load()		- Load an Article object.
//   article::loadform()	- Load an Article object from form data.
//   article::save()		- Save an Article object.
//   article::search()		- Get a list of Article IDs.
//   article::validate()	- Validate the current Article object values.
//   article::view()		- View the current Article object.
//

include_once "db.php";


// Article sections...
$ARTICLE_SECTIONS = array();
$result = db_query("SELECT DISTINCT section FROM article ORDER BY section");
while ($row = db_next($result))
  $ARTICLE_SECTIONS[sizeof($ARTICLE_SECTIONS)] = $row["section"];
db_free($result);


class article
{
  //
  // Instance variables...
  //

  var $id;
  var $link_id;
  var $is_published;
  var $section, $section_valid;
  var $title, $title_valid;
  var $abstract, $abstract_valid;
  var $contents, $contents_valid;
  var $create_date;
  var $create_user;
  var $modify_date;
  var $modify_user;


  //
  // 'article::article()' - Create an Article object.
  //

  function				// O - New Article object
  article($id = 0)			// I - ID, if any
  {
    if ($id > 0)
      $this->load($id);
    else
      $this->clear();
  }


  //
  // 'article::clear()' - Initialize a new an Article object.
  //

  function
  clear()
  {
    $this->id = 0;
    $this->link_id = 0;
    $this->is_published = 0;
    $this->section = "";
    $this->title = "";
    $this->abstract = "";
    $this->contents = "";
    $this->create_date = 0;
    $this->create_user = "";
    $this->modify_date = 0;
    $this->modify_user = "";
  }


  //
  // 'article::delete()' - Delete an Article object.
  //

  function
  delete()
  {
    db_query("DELETE FROM article WHERE id=$this->id");
    $this->clear();
  }


  //
  // 'article::edit()' - Display a form for an Article object.
  //

  function
  edit($options = "")			// I - Search/etc. options
  {
    global $ARTICLE_SECTIONS, $LOGIN_USER, $LOGIN_LEVEL, $PHP_SELF, $REQUEST_METHOD;


    if ($this->id <= 0)
      $action = "Submit Article";
    else
      $action = "Modify Article #$this->id";

    print("<h1>$action</h1>\n"
         ."<form action='$PHP_SELF?U$this->id$options' method='POST'>\n"
         ."<p><table class='edit'>\n");

    // is_published
    print("<tr><th class='valid' align='right' valign='top'>Published:</th><td>");
    html_select_is_published($this->is_published);
    print("</td></tr>\n");

    // section
    if (array_search($this->section, $ARTICLE_SECTIONS) === FALSE)
      $html = htmlspecialchars($this->section, ENT_QUOTES);
    else
      $html = "";

    if ($this->section_valid || $REQUEST_METHOD == "GET")
      $hclass = "valid";
    else
      $hclass = "invalid";
    print("<tr><th class='$hclass' align='right' valign='top'>Section:</th>"
         ."<td><select name='section'><option value=''>Unassigned</option>");
    reset($ARTICLE_SECTIONS);
    while (list($key, $val) = each($ARTICLE_SECTIONS))
    {
      print("<option value='$val'");
      if ($this->section == $val)
	print(" selected");
      print(">$val</option>");
    }
    print("<option value=''>Other...</option></select><br>\n"
         ."Other: <input type='text' name='section_other' value='$html' "
	 ."size='40' maxsize='255'></td></tr>\n");

    // title
    $html = htmlspecialchars($this->title, ENT_QUOTES);
    if ($this->title_valid || $REQUEST_METHOD == "GET")
      $hclass = "valid";
    else
      $hclass = "invalid";
    print("<tr><th class='$hclass' align='right' valign='top'>Title:</th><td>");
    print("<input type='text' name='title' value='$html' size='72' "
         ."maxsize='255'>");
    print("</td></tr>\n");

    // abstract
    $html = htmlspecialchars($this->abstract, ENT_QUOTES);
    if ($this->abstract_valid || $REQUEST_METHOD == "GET")
      $hclass = "valid";
    else
      $hclass = "invalid";
    print("<tr><th class='$hclass' align='right' valign='top'>Abstract:</th><td>");
    print("<textarea name='abstract' cols='72' rows='2' wrap='virtual'>"
         ."$html</textarea></td></tr>\n");

    // contents
    $html = htmlspecialchars($this->contents, ENT_QUOTES);
    if ($this->contents_valid || $REQUEST_METHOD == "GET")
      $hclass = "valid";
    else
      $hclass = "invalid";
    print("<tr><th class='$hclass' align='right' valign='top'>Contents:</th><td>");
    print("<textarea name='contents' cols='72' rows='20' wrap='virtual'>$html</textarea>\n"
         ."<p>The contents of the article may contain the following "
         ."HTML elements: <tt>A</tt>, <tt>B</tt>, <tt>BLOCKQUOTE</tt>, "
         ."<tt>CODE</tt>, <tt>EM</tt>, <tt>H1</tt>, <tt>H2</tt>, "
         ."<tt>H3</tt>, <tt>H4</tt>, <tt>H5</tt>, <tt>H6</tt>, <tt>I</tt>, "
         ."<tt>IMG</tt>, <tt>LI</tt>, <tt>OL</tt>, <tt>P</tt>, <tt>PRE</tt>, "
         ."<tt>SUP</tt>, <tt>TT</tt>, <tt>U</tt>, <tt>UL</tt></p>\n");
    print("</td></tr>\n");

    // Submit
    print("<tr><td></td><td>"
         ."<input type='submit' name='SUBMIT' value='$action'>"
         ."<input type='submit' name='SUBMIT' value='Preview Article'>"
         ."</td></tr>\n"
         ."</table></p>\n"
         ."</form>\n");
  }


  //
  // 'article::load()' - Load an Article object.
  //

  function				// O - TRUE if OK, FALSE otherwise
  load($id)				// I - Object ID
  {
    $this->clear();

    $result = db_query("SELECT * FROM article WHERE id = $id");
    if (db_count($result) != 1)
      return (FALSE);

    $row = db_next($result);
    $this->id = $row["id"];
    $this->is_published = $row["is_published"];
    $this->section = $row["section"];
    $this->title = $row["title"];
    $this->abstract = $row["abstract"];
    $this->contents = $row["contents"];
    $this->create_date = $row["create_date"];
    $this->create_user = $row["create_user"];
    $this->modify_date = $row["modify_date"];
    $this->modify_user = $row["modify_user"];

    db_free($result);

    return ($this->validate());
  }


  //
  // 'article::loadform()' - Load an Article object from form data.
  //

  function				// O - TRUE if OK, FALSE otherwise
  loadform()
  {
    global $_GET, $_POST, $LOGIN_LEVEL;


    if ($LOGIN_LEVEL < AUTH_DEVEL && $this->id == 0)
      $this->is_published = 0;
    else if (array_key_exists("is_published", $_GET))
      $this->is_published = $_GET["is_published"];
    else if (array_key_exists("is_published", $_POST))
      $this->is_published = $_POST["is_published"];

    if (array_key_exists("section", $_GET))
      $this->section = $_GET["section"];
    else if (array_key_exists("section", $_POST))
      $this->section = $_POST["section"];

    if ($this->section == "")
    {
      if (array_key_exists("section_other", $_GET))
	$this->section = $_GET["section_other"];
      else if (array_key_exists("section_other", $_POST))
	$this->section = $_POST["section_other"];
    }

    if (array_key_exists("title", $_GET))
      $this->title = $_GET["title"];
    else if (array_key_exists("title", $_POST))
      $this->title = $_POST["title"];

    if (array_key_exists("abstract", $_GET))
      $this->abstract = $_GET["abstract"];
    else if (array_key_exists("abstract", $_POST))
      $this->abstract = $_POST["abstract"];

    if (array_key_exists("contents", $_GET))
      $this->contents = $_GET["contents"];
    else if (array_key_exists("contents", $_POST))
      $this->contents = $_POST["contents"];

    return ($this->validate());
  }


  //
  // 'article::save()' - Save an Article object.
  //

  function				// O - TRUE if OK, FALSE otherwise
  save()
  {
    global $LOGIN_USER, $PHP_SELF;


    $this->modify_date = time();
    $this->modify_user = $LOGIN_USER;

    if ($this->id > 0)
    {
      return (db_query("UPDATE article "
                      ." SET is_published = $this->is_published"
                      .", section = '" . db_escape($this->section) . "'"
                      .", title = '" . db_escape($this->title) . "'"
                      .", abstract = '" . db_escape($this->abstract) . "'"
                      .", contents = '" . db_escape($this->contents) . "'"
                      .", modify_date = $this->modify_date"
                      .", modify_user = '" . db_escape($this->modify_user) . "'"
                      ." WHERE id = $this->id") !== FALSE);
    }
    else
    {
      $this->create_date = time();
      $this->create_user = $LOGIN_USER;

      if (db_query("INSERT INTO article VALUES"
                  ."(NULL"
		  .", $this->link_id"
                  .", $this->is_published"
                  .", '" . db_escape($this->section) . "'"
                  .", '" . db_escape($this->title) . "'"
                  .", '" . db_escape($this->abstract) . "'"
                  .", '" . db_escape($this->contents) . "'"
                  .", $this->create_date"
                  .", '" . db_escape($this->create_user) . "'"
                  .", $this->modify_date"
                  .", '" . db_escape($this->modify_user) . "'"
                  .")") === FALSE)
        return (FALSE);

      $this->id = db_insert_id();
    }

    return (TRUE);
  }


  //
  // 'article::search()' - Get a list of Article objects.
  //

  function				// O - Array of Article objects
  search($search = "",			// I - Search string
         $order = "",			// I - Order fields
	 $section = "",			// I - Section field
	 $is_published = 0)		// I - Only return published articles
  {
    global $LOGIN_USER, $LOGIN_LEVEL;


    $query  = "";
    $prefix = " WHERE ";

    if ($is_published)
    {
      $query .= "${prefix}is_published = 1";
      $prefix = " AND ";
    }
    else if ($LOGIN_LEVEL < AUTH_DEVEL)
    {
      $query .= "${prefix}(is_published = 1 OR create_user = '"
	       . db_escape($LOGIN_USER) . "')";
      $prefix = " AND ";
    }

    if ($section)
    {
      if ($section[0] == '!')
	$query .= "${prefix}section <> '" . db_escape(substr($section, 1)) . "'";
      else if ($section == "Mine")
	$query .= "${prefix}create_user = '" . db_escape($LOGIN_USER) . "'";
      else
	$query .= "${prefix}section = '" . db_escape($section) . "'";

      $prefix = " AND ";
    }

    if ($search != "")
    {
      // Convert the search string to an array of words...
      $words = html_search_words($search);

      // Loop through the array of words, adding them to the query...
      $query .= "${prefix}(";
      $prefix = "";
      $next   = " OR";
      $logic  = "";

      reset($words);
      foreach ($words as $word)
      {
        if ($word == "or")
        {
          $next = ' OR';
          if ($prefix != '')
            $prefix = ' OR';
        }
        else if ($word == "and")
        {
          $next = ' AND';
          if ($prefix != '')
            $prefix = ' AND';
        }
        else if ($word == "not")
          $logic = ' NOT';
        else if (substr($word, 0, 8) == "creator:")
	{
	  $word   = substr($word, 8);
          $query .= "$prefix$logic create_user LIKE \"$word\"";
          $prefix = $next;
          $logic  = '';
	}
        else if (substr($word, 0, 7) == "number:")
	{
	  $number = (int)substr($word, 7);
          $query  .= "$prefix$logic id = $number";
          $prefix = $next;
          $logic  = '';
	}
        else if (substr($word, 0, 6) == "title:")
	{
	  $word   = substr($word, 6);
          $query  .= "$prefix$logic title LIKE \"%$word%\"";
          $prefix = $next;
          $logic  = '';
	}
        else
        {
          $query .= "$prefix$logic (";
          $subpre = "";

          if (ereg("^[0-9]+\$", $word))
          {
            $query .= "${subpre}id = $word";
            $subpre = " OR ";
          }

          $query .= "${subpre}title LIKE \"%$word%\"";
          $subpre = " OR ";
          $query .= "${subpre}abstract LIKE \"%$word%\"";
          $query .= "${subpre}contents LIKE \"%$word%\"";

          $query .= ")";
          $prefix = $next;
          $logic  = '';
        }
      }

      $query .= ")";
    }

    if ($order != "")
    {
      // Separate order into array...
      $fields = explode(" ", $order);
      $prefix = " ORDER BY ";

      // Add ORDER BY stuff...
      foreach ($fields as $field)
      {
        if ($field[0] == '+')
          $query .= "${prefix}" . substr($field, 1);
        else if ($field[0] == '-')
          $query .= "${prefix}" . substr($field, 1) . " DESC";
        else
          $query .= "${prefix}$field";

        $prefix = ", ";
      }
    }

//    print("<p>$query</p>\n");

    // Do the query and convert the result to an array of objects...
    $result  = db_query("SELECT id FROM article$query");
    $matches = array();

    while ($row = db_next($result))
      $matches[sizeof($matches)] = $row["id"];

    // Free the query result and return the array...
    db_free($result);

    return ($matches);
  }


  //
  // 'article::validate()' - Validate the current Article object values.
  //

  function				// O - TRUE if OK, FALSE otherwise
  validate()
  {
    $valid = TRUE;

    if ($this->title == "")
    {
      $this->title_valid = FALSE;
      $valid = FALSE;
    }
    else
      $this->title_valid = TRUE;

    if ($this->abstract == "")
    {
      $this->abstract_valid = FALSE;
      $valid = FALSE;
    }
    else
      $this->abstract_valid = TRUE;

    if ($this->section == "")
    {
      $this->section_valid = FALSE;
      $valid = FALSE;
    }
    else
      $this->section_valid = TRUE;

    if ($this->contents == "")
    {
      $this->contents_valid = FALSE;
      $valid = FALSE;
    }
    else
      $this->contents_valid = TRUE;

    return ($valid);
  }


  //
  // 'article::view()' - View the current Article object.
  //

  function
  view()
  {
    global $html_path;

    $section     = htmlspecialchars($this->section);
    $title       = htmlspecialchars($this->title);
    $contents    = html_format($this->contents);
    $create_user = sanitize_email($this->create_user);
    $create_date = date("H:i M d, Y", $this->create_date);
    $modify_user = sanitize_email($this->modify_user);
    $modify_date = date("H:i M d, Y", $this->modify_date);

    if (!$this->is_published)
      print("<p align='center'><b>This article is currently hidden from "
	   ."public view.</b></p>\n");

    print("<h1>Article #$this->id: $title</h1>\n"
	 ."<p><i>Created at $create_date by $create_user</i><br>");
    if ($modify_date != $create_date && $modify_user == $create_user)
      print("<p><i>Last modified at $modify_date</i><br>");
    print("$contents\n");

    print("<hr noshade>\n"
	 ."<h2><a name='_USER_COMMENTS'>Comments</a></h2>\n");

    html_start_links();
    html_link("Submit Comment", "comment.php?U+R0+Particles.php_L$this->id");
    html_end_links();

    show_comments("articles.php_L$this->id");
  }
}


//
// End of "$Id$".
//
?>
