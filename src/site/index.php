<?php
	$app['sitename'] = "Blood Frontier";
	$app['siteblurb'] = "It's Bloody Fun!";
	$app['siterelver'] = "v0.80 (Beta 1)";
	$app['sitereldate'] = "24th February 2009";
	$app['sitedonate'] = "<div id=\"right\"><form action=\"https://www.paypal.com/cgi-bin/webscr\" method=\"post\"><input type=\"hidden\" name=\"cmd\" value=\"_s-xclick\"><input type=\"hidden\" name=\"hosted_button_id\" value=\"212900\"><input type=\"image\" src=\"https://www.paypal.com/en_AU/i/btn/btn_donate_LG.gif\" name=\"submit\" alt=\"Donate to Quin\"></form></div>";
	$app['sitenotice'] = "We need your <u>help</u> to keep making this game as awesome as possible; the Lead Developer, Quinton \"Quin\" Reeves is unemployed and struggles with chronic illness, despite this he continues to work effortlessly on this project. Your <u>donation</u> can improve the quality of his life; please consider being generous, and make sure he can keep doing this important task!";
	$app['sitevideo'] = "http://www.youtube.com/v/DHNXAwVXF8g&amp;color1=0x000000&amp;color2=0x000000&amp;border=0&amp;fs=1&amp;egm=0&amp;showsearch=0&amp;showinfo=0&amp;ap=%2526fmt%3D18";
	$app['siteinfo'] = "In the distant future, humanity has spread throughout the solar system, to Mars and beyond. A vast communications network bridges from colony to colony, human to machine, and machine to human. This seemingly benign keystone of modern inter-planetary society, however, appears to be the carrier of a mysterious techno-biological plague. Any persons so-connected seem to fall ill and die, only to return as ravenous, sub-human cannibals. You, a machine intelligence, an android, remain unafflicted by this strange phenomenon and have been tasked with destroying the growing hordes of the infected, while, hopefully, locating and stopping the source of the epidemic.";
	$app['sitemainlogo'] = "/bits/logo.png";
	$app['sitecss'] = "/bits/site.css";
	$app['siteico'] = "/bits/favicon.ico";

	$app['siteaddname'] = "Built on Cube Engine 2";
	$app['siteaddurl'] = "http://www.cubeengine.com/";
	$app['siteaddlogo'] = "/bits/cube2.png";

	$app['sfproject'] = "bloodfrontier";
	$app['sfgroupid'] = 198419;
	$app['sflogo'] = 11;
	$app['sfpiwik'] = 1; // use SF's piwik with idsite=N

	$app['ircnetwork'] = "irc.freenode.net";
	$app['ircchannel'] = "bloodfrontier";
	$app['ircsetup'] = "bloodfrontier";

	$app['deftarg'] = "home";
	$app['defsearch'] = "%22Blood%20Frontier%22";

	$app['targets'] = array('home' => array('name' => 'Home', 'url' => '/', 'alturl' => '', 'nav' => 0, 'redir' => 0));
	$app['targets']['download'] = array('name' => 'Download', 'url' => 'http://sourceforge.net/project/platformdownload.php?group_id='.$app['sfgroupid'], 'alturl' => 'http://sourceforge.net/project/platformdownload.php?group_id='.$app['sfgroupid'].'&amp;sel_platform=', 'nav' => 1, 'redir' => 1);
	$app['targets']['blog'] = array('name' => 'Blog', 'url' => 'http://apps.sourceforge.net/wordpress/'.$app['sfproject'].'/', 'alturl' => 'http://apps.sourceforge.net/wordpress/'.$app['sfproject'].'/', 'nav' => 1, 'redir' => 1);
	$app['targets']['wiki'] = array('name' => 'Wiki', 'url' => 'http://apps.sourceforge.net/mediawiki/'.$app['sfproject'].'/', 'alturl' => 'http://apps.sourceforge.net/mediawiki/'.$app['sfproject'].'/index.php?title=', 'nav' => 1, 'redir' => 1);
	$app['targets']['forums'] = array('name' => 'Forums', 'url' => 'http://apps.sourceforge.net/phpbb/'.$app['sfproject'].'/', 'alturl' => 'http://apps.sourceforge.net/phpbb/'.$app['sfproject'].'/viewforum.php?f=', 'nav' => 1, 'redir' => 1);
	$app['targets']['chat'] = array('name' => 'Chat', 'url' => 'http://embed.mibbit.com/?server='.$app['ircnetwork'].'&amp;channel=%23'.$app['ircchannel'].'&amp;settings='.$app['ircsetup'].'&amp;forcePrompt=true', 'alturl' => '', 'nav' => 1, 'redir' => 1);
	$app['targets']['gallery'] = array('name' => 'Gallery', 'url' => 'http://apps.sourceforge.net/gallery/'.$app['sfproject'].'/', 'alturl' => 'http://apps.sourceforge.net/gallery/'.$app['sfproject'].'/index.php?g2_itemId=', 'nav' => 1, 'redir' => 1);
	$app['targets']['project'] = array('name' => 'Project', 'url' => 'http://sourceforge.net/projects/'.$app['sfproject'].'/', 'alturl' => '', 'nav' => 1, 'redir' => 1);
	$app['targets']['svn'] = array('name' => 'SVN', 'url' => 'http://'.$app['sfproject'].'.svn.sourceforge.net/'.$app['sfproject'].'/', 'alturl' => 'http://'.$app['sfproject'].'.svn.sourceforge.net/viewvc/'.$app['sfproject'].'/?view=rev&amp;rev=', 'nav' => 1, 'redir' => 1);
	$app['targets']['repo'] = array('name' => 'Repository', 'url' => 'http://'.$app['sfproject'].'.svn.sourceforge.net/viewvc/'.$app['sfproject'], 'alturl' => 'http://'.$app['sfproject'].'.svn.sourceforge.net/viewvc/'.$app['sfproject'].'/', 'nav' => 0, 'redir' => 1);
	$app['targets']['facebook'] = array('name' => 'On Facebook', 'url' => 'http://www.facebook.com/group.php?gid=86675052192&amp;ref=mf', 'nav' => 1, 'redir' => 1);
	$app['targets']['google'] = array('name' => 'On Google', 'url' => 'http://www.google.com/search?q='.$app['defsearch'], 'alturl' => 'http://www.google.com/search?q='.$app['defsearch'].'+', 'nav' => 1, 'redir' => 1);
	$app['targets']['youtube'] = array('name' => 'On YouTube', 'url' => 'http://www.youtube.com/results?search_query='.$app['defsearch'], 'alturl' => 'http://www.youtube.com/results?search_query='.$app['defsearch'].'+', 'nav' => 1, 'redir' => 1);

	function checkarg($arg = "", $def = "") {
		return isset($_GET[$arg]) && $_GET[$arg] != "" ? $_GET[$arg] : $def;
	}
	
	$app['target'] = checkarg("target", $app['deftarg']);
	if (!isset($app['targets'][$app['target']])) $app['target'] = $app['deftarg'];

	$title = checkarg("title");
	if ($app['targets'][$app['target']]['redir']) {
		$app['url'] = $title != "" ? (
				$app['targets'][$app['target']]['alturl'] != "" ? $app['targets'][$app['target']]['alturl'].$title : $app['targets'][$app['target']]['url'].$title
		) : $app['targets'][$app['target']]['url'];
		header("Location: ".$app['url']);
		exit;
	}
	else {
		$app['url'] = $title != "" ? (
				$app['targets'][$app['target']]['alturl'] != "" ? $app['targets'][$app['target']]['alturl'].$title : $app['targets'][$app['target']]['url'].$title
		) : $app['targets'][$app['target']]['url'];
		$app['navbar'] = ''; // cache the navbar
		foreach ($app['targets'] as $key => $targ) {
			if ($key != "" && $targ['name'] != "" && $targ['nav']) {
				$app['navbar'] .= '<li><a href="/'.$key.'">'. $targ['name'] .'</a></li>';
			}
		}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en" dir="ltr">
	<head>
		<title><?php echo $app['sitename'].": ".($app['targets'][$app['target']]['name'] != "" ? $app['targets'][$app['target']]['name'] : "Welcome"); ?></title>
		<meta http-equiv="Content-Type" content="text/html;charset=utf-8" />
		<link rel="shortcut icon" href="<?php echo $app['siteico']; ?>" />
		<link rel="stylesheet" type="text/css" href="<?php echo $app['sitecss']; ?>" />
<?php if ($app['sfpiwik'] > 0) { ?>
		<script type="text/javascript"><!--
			var pkBaseURL = (("https:" == document.location.protocol) ? "https://apps.sourceforge.net/piwik/<?php echo $app['sfproject']; ?>/" : "http://apps.sourceforge.net/piwik/<?php echo $app['sfproject']; ?>/");
			document.write(unescape("%3Cscript src='" + pkBaseURL + "piwik.js' type='text/javascript'%3E%3C/script%3E"));
			piwik_action_name = '<?php echo $app['target']; ?>';
			piwik_idsite = 1;
			piwik_url = pkBaseURL + "piwik.php";
			piwik_log(piwik_action_name, piwik_idsite, piwik_url);
		--></script>
		<object><noscript><p><img src="http://apps.sourceforge.net/piwik/<?php echo $app['sfproject']; ?>/piwik.php?idsite=<?php echo $app['sfpiwik']; ?>" alt="piwik"/></p></noscript></object>
<?php } ?>
	</head>
	<body>
		<div id="wrapper" align="center">
			<div id="header">
				<div id="logo">
					<a href="/home"><img id="sitelogo" src="<?php echo $app['sitemainlogo']; ?>" alt="<?php echo $app['sitename']; ?>" style="border: none" /></a>
					<a href="<?php echo $app['siteaddurl']; ?>"><img id="sitelogo" src="<?php echo $app['siteaddlogo']; ?>" alt="<?php echo $app['siteaddname']; ?>" style="border: none" /></a>
				</div>
				<div id="tags">
<?php			if ($app['sflogo'] > 0) { ?>
					<a href="/project">
						<img id="sflogo" src="http://sflogo.sourceforge.net/sflogo.php?group_id=<?php echo $app['sfgroupid']; ?>&amp;type=<?php echo $app['sflogo']; ?>" style="border: none" alt="Get <?php echo $app['sitename']; ?> at SourceForge" />
					</a>
<?php			} ?>
					<div id="blurb"><i><?php echo $app['siteblurb']; ?></i></div>
				</div>
			</div>
			<div id="version"><b>Current Version:</b> <a href="/download"><?php echo $app['siterelver']; ?></a> released <i><?php echo $app['sitereldate']; ?></i></div>
			<div id="navbar">
				<ul class="nav-list">
					<?php echo $app['navbar']; ?>
				</ul>
			</div>
			<div id="notice" align="center">
<?php 			echo $app['sitedonate'] . $app['sitenotice']; ?>
			</div>
			<div id="body" align="center">
<?php			include_once($app['target'].'.php'); ?>
			</div>
		</div>
	</body>
</html>
<?php } ?>