<?php
	$app['sitename'] = "Blood Frontier";
	$app['siteblurb'] = "It's Bloody Fun!";
	$app['siterelver'] = "v0.80 (Beta 1)";
	$app['sitereldate'] = "24th February 2009";
	$app['sitevideo'] = "http://www.youtube.com/v/DHNXAwVXF8g&amp;color1=0x000000&amp;color2=0x000000&amp;border=0&amp;fs=1&amp;egm=0&amp;showsearch=0&amp;showinfo=0&amp;ap=%2526fmt%3D18";
	$app['sitenotice'] = "Blood Frontier is a Free and Open Source game, using SDL and OpenGL which allows it to be ported to many platforms; you can download it for both PC (Windows, Linux, BSD) and Mac. You can also obtain it through our subversion repository and live on the bleeding edge of development. The game is a single-player and multi-player first-person shooter, built as a total conversion of Cube Engine 2, which lends itself toward a balanced gameplay, completely at the control of map makers, while maintaining a general theme of tactics and low gravity. In a true open source <i>by the people for the people</i> nature, it tries to work closely with the gaming and open source communities to provide a better overall experience, taking all sorts of feedback from your average player, to your seasoned developer, aiming to create a story-driven game environment that is flexible, fun, easy to use, and pleasing to the eye.";
	$app['siteinfo'] = "In the distant future, humanity has spread throughout the solar system, to Mars and beyond. A vast communications network bridges from colony to colony, human to machine, and machine to human. This seemingly benign keystone of modern inter-planetary society, however, appears to be the carrier of a mysterious techno-biological plague. Any persons so-connected seem to fall ill and die, only to return as ravenous, sub-human cannibals. You, a machine intelligence, an android, remain unafflicted by this strange phenomenon and have been tasked with destroying the growing hordes of the infected, while, hopefully, locating and stopping the source of the epidemic.";
	$app['sitepaypal'] = "212900";
	$app['sitedonate'] = "Donate to Quin";
	$app['sitemainlogo'] = "/bits/logo_bf.png";
	$app['sitecss'] = "/bits/style.css";
	$app['siteico'] = "/bits/favicon.ico";
	$app['sitemiddle'] = "/bits/middle.png";
	$app['siteaddname'] = "Built on Cube Engine 2";
	$app['siteaddurl'] = "http://www.cubeengine.com/";
	$app['siteaddlogo'] = "/bits/logo_c2.png";

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
	$app['targets']['facebook'] = array('name' => 'On Facebook', 'url' => 'http://www.facebook.com/group.php?gid=86675052192&amp;ref=mf', 'nav' => 0, 'redir' => 1);
	$app['targets']['google'] = array('name' => 'On Google', 'url' => 'http://www.google.com/search?q='.$app['defsearch'], 'alturl' => 'http://www.google.com/search?q='.$app['defsearch'].'+', 'nav' => 0, 'redir' => 1);
	$app['targets']['youtube'] = array('name' => 'On YouTube', 'url' => 'http://www.youtube.com/results?search_query='.$app['defsearch'], 'alturl' => 'http://www.youtube.com/results?search_query='.$app['defsearch'].'+', 'nav' => 0, 'redir' => 1);

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
				$app['navbar'] .= '<a href="/'.$key.'">'. $targ['name'] .'</a> ';
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
		--></script>
		<script type="text/javascript"><!--
			piwik_action_name = '<?php echo $app['target']; ?>';
			piwik_idsite = 1;
			piwik_url = pkBaseURL + "piwik.php";
			piwik_log(piwik_action_name, piwik_idsite, piwik_url);
		--></script>
		<object><noscript><p><img src="http://apps.sourceforge.net/piwik/<?php echo $app['sfproject']; ?>/piwik.php?idsite=<?php echo $app['sfpiwik']; ?>" alt="piwik"/></p></noscript></object>
<?php } ?>
	</head>
	<body>
		<div id="container">
			<div id="header">
				<a href="/home"><img src="<?php echo $app['sitemainlogo']; ?>" alt="<?php echo $app['sitename']; ?>" title="<?php echo $app['sitename']; ?>" width="304" height="143" border="0" /></a>
				<img src="<?php echo $app['sitemiddle']; ?>" alt="<?php echo $app['siteblurb']; ?>" title="<?php echo $app['siteblurb']; ?>" width="323" height="143" border="0" />
				<a href="<?php echo $app['siteaddurl']; ?>"><img src="<?php echo $app['siteaddlogo']; ?>" alt="<?php echo $app['siteaddname']; ?>" title="<?php echo $app['siteaddname']; ?>" width="193" height="143" border="0" /></a>
			</div>
			<div id="page">
				<div id="links"><?php echo $app['navbar']; ?></div>
				<div id="noticebg">
					<div id="list">
						<h2><?php echo $app['sitename']; ?></h2>
						<ul>
							<li><a href="/youtube">on Youtube</a></li>
							<li><a href="/google">on Google</a></li>
							<li><a href="/facebook">on Facebook</a></li>
						</ul>
<?php				if ($app['sflogo'] > 0) { ?>
						<div id="listlogo">
							<a href="/project">
								<img src="http://sflogo.sourceforge.net/sflogo.php?group_id=<?php echo $app['sfgroupid']; ?>&amp;type=<?php echo $app['sflogo']; ?>" alt="Get <?php echo $app['sitename']; ?> at SourceForge" title="Get <?php echo $app['sitename']; ?> at SourceForge" width="120" height="30" border="0" id="sflogo"/>
							</a>
						</div>
<?php				} ?>
						<div id="listdonate">
							<form method="post" action="https://www.paypal.com/cgi-bin/webscr">
								<input type="hidden" value="_s-xclick" name="cmd"/>
								<input type="hidden" value="<?php echo $app['sitepaypal']; ?>" name="hosted_button_id"/>
								<input type="image" alt="<?php echo $app['sitedonate']; ?>" name="submit" src="https://www.paypal.com/en_AU/i/btn/btn_donate_LG.gif"/>
							</form>
						</div>
					</div>
					<img src="/bits/block_up.png" alt="" width="549" height="20" border="0" id="blockborder" />
					<div id="redblock" align="center">
						<p id="version">Current Version: <b><a href="/download"><?php echo $app['siterelver']; ?></a></b> released <i><?php echo $app['sitereldate']; ?></i></p>
						<hr />
						<p id="notice"><?php echo $app['sitenotice']; ?></p>
					</div>
					<img src="/bits/block_down.png" alt="" width="549" height="20" border="0" id="blockborder" />
				</div>
				<div id="video">
					<div id="youtube">
						<object width="480" height="385" type="application/x-shockwave-flash" data="<?php echo $app['sitevideo']; ?>">
							<param name="movie" value="<?php echo $app['sitevideo']; ?>" />
							<param name="allowscriptaccess" value="always" />
							<param name="allowFullScreen" value="true" />
						</object>
					</div>
					<div id="subtext" align="center">
						<h2><?php echo $app['sitename']; ?></h2>
						<h3><?php echo $app['siteblurb']; ?></h3>
						<p><?php echo $app['siteinfo']; ?></p>
						<p>
							<a href="/bits/ss01.jpg"><img src="/bits/ss01t.jpg" alt="Screenshot 01" width="85" height="53" border="0" /></a>
							<a href="/bits/ss02.jpg"><img src="/bits/ss02t.jpg" alt="Screenshot 02" width="85" height="53" border="0" /></a>
							<a href="/bits/ss03.jpg"><img src="/bits/ss03t.jpg" alt="Screenshot 03" width="85" height="53" border="0" /></a>
							<a href="/bits/ss04.jpg"><img src="/bits/ss04t.jpg" alt="Screenshot 04" width="85" height="53" border="0" /></a>
						</p>
						<p><a href="/gallery">View Gallery</a></p>
					</div>
				</div>
			</div>
			<div id="footer"><a href="/download">Download</a>, <a href="/wiki">Learn More</a>, <a href="/forums">Get Help</a>, or <a href="/chat">Join In</a> today!</div>
		</div>
	</body>
</html>
<?php } ?>
