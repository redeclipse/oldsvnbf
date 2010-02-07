<?php
    $app['releasever'] = "v0.85 (Beta 2)";
    $app['releasedate'] = "6th December 2009";
    $app['background'] = "/bits/background_01.jpg";

    $app['targets'] = array('home' => array('name' => '', 'url' => '/', 'alturl' => '', 'nav' => -1, 'redir' => 0));
    $app['targets']['download'] = array('name' => 'Download', 'url' => 'https://sourceforge.net/projects/bloodfrontier/files/', 'alturl' => 'https://sourceforge.net/projects/bloodfrontier/files/', 'nav' => 1, 'redir' => 1);
    $app['targets']['blog'] = array('name' => 'Blog', 'url' => 'http://sourceforge.net/apps/wordpress/bloodfrontier/', 'alturl' => 'http://sourceforge.net/apps/wordpress/bloodfrontier/', 'nav' => 1, 'redir' => 1);
    $app['targets']['wiki'] = array('name' => 'Wiki', 'url' => 'http://sourceforge.net/apps/mediawiki/bloodfrontier/', 'alturl' => 'http://sourceforge.net/apps/mediawiki/bloodfrontier/index.php?title=', 'nav' => 1, 'redir' => 1);
    $app['targets']['forums'] = array('name' => 'Forums', 'url' => 'http://sourceforge.net/apps/phpbb/bloodfrontier/', 'alturl' => 'http://sourceforge.net/apps/phpbb/bloodfrontier/viewforum.php?f=', 'nav' => 1, 'redir' => 1);
    $app['targets']['chat'] = array('name' => 'Chat', 'url' => 'http://webchat.freenode.net/?channels=bloodfrontier', 'alturl' => '', 'nav' => 1, 'redir' => 1);
    $app['targets']['gallery'] = array('name' => 'Gallery', 'url' => 'http://sourceforge.net/apps/gallery/bloodfrontier/', 'alturl' => 'http://sourceforge.net/apps/gallery/bloodfrontier/index.php?g2_itemId=', 'nav' => 1, 'redir' => 1);
    $app['targets']['project'] = array('name' => 'Project', 'url' => 'http://sourceforge.net/projects/bloodfrontier/', 'alturl' => '', 'nav' => 1, 'redir' => 1);
    $app['targets']['svn'] = array('name' => 'SVN', 'url' => 'http://bloodfrontier.svn.sourceforge.net/bloodfrontier/', 'alturl' => 'http://bloodfrontier.svn.sourceforge.net/viewvc/bloodfrontier/?view=rev&amp;rev=', 'nav' => 1, 'redir' => 1);
    $app['targets']['repo'] = array('name' => 'Repository', 'url' => 'http://bloodfrontier.svn.sourceforge.net/viewvc/bloodfrontier', 'alturl' => 'http://bloodfrontier.svn.sourceforge.net/viewvc/bloodfrontier/', 'nav' => -1, 'redir' => 1);
    $app['targets']['youtube'] = array('name' => 'youtube', 'url' => 'http://www.youtube.com/results?search_query=%22Blood%20Frontier%22', 'alturl' => 'http://www.youtube.com/results?search_query=%22Blood%20Frontier%22+', 'nav' => 0, 'redir' => 1);
    $app['targets']['google'] = array('name' => 'google', 'url' => 'http://www.google.com/search?q=%22Blood%20Frontier%22', 'alturl' => 'http://www.google.com/search?q=%22Blood%20Frontier%22+', 'nav' => 0, 'redir' => 1);
    $app['targets']['facebook'] = array('name' => 'facebook', 'url' => 'http://www.facebook.com/group.php?gid=86675052192&amp;ref=mf', 'nav' => 0, 'redir' => 1);
    $app['targets']['blackboxbeta'] = array('name' => 'blackboxbeta', 'url' => 'http://www.blackboxbeta.net/blood-frontier-oss', 'nav' => 0, 'redir' => 1);
    $app['targets']['playdeb'] = array('name' => 'playdeb', 'url' => 'http://www.playdeb.net/updates/Blood%20Frontier', 'nav' => 0, 'redir' => 1);

    function checkarg($arg = "", $def = "") {
        return isset($_GET[$arg]) && $_GET[$arg] != "" ? $_GET[$arg] : $def;
    }
    
    $app['target'] = checkarg("target", "home");
    if (!isset($app['targets'][$app['target']])) $app['target'] = "home";

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
            if ($key != "" && $targ['name'] != "" && $targ['nav'] == 1) {
                $app['navbar'] .= '<a href="/'.$key.'">'. $targ['name'] .'</a> ';
            }
        }
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en" dir="ltr">
    <head>
        <title>Blood Frontier<?php echo ($app['targets'][$app['target']]['name'] != "" ? ": ".$app['targets'][$app['target']]['name'] : ", It's Bloody Fun!"); ?></title>
        <meta http-equiv="Content-Type" content="text/html;charset=utf-8" />
        <link rel="shortcut icon" href="/bits/favicon.ico" />
        <link rel="stylesheet" type="text/css" href="/bits/style.css" />
        <script type="text/javascript" src="/bits/js/jquery.js"></script>
        <script type="text/javascript" src="/bits/js/jquery.lightbox-0.5.js"></script>
        <script type="text/javascript">
            $(function() {
                $('#gallery a').lightBox();
            });
        </script>
        <script type="text/javascript" src="/bits/js/easySlider1.7.js"></script>
        <script type="text/javascript">
            $(document).ready(function(){   
                $("#slider").easySlider({
            auto: true,
            continuous: true,
            nextId: "slider1next",
            prevId: "slider1prev"
                });
            
            }); 
        </script>
        <script type="text/javascript"><!--
            var pkBaseURL = (("https:" == document.location.protocol) ? "https://sourceforge.net/apps/piwik/bloodfrontier/" : "http://sourceforge.net/apps/piwik/bloodfrontier/");
            document.write(unescape("%3Cscript src='" + pkBaseURL + "piwik.js' type='text/javascript'%3E%3C/script%3E"));
        --></script>
        <script type="text/javascript"><!--
            piwik_action_name = '<?php echo $app['target']; ?>';
            piwik_idsite = 1;
            piwik_url = pkBaseURL + "piwik.php";
            piwik_log(piwik_action_name, piwik_idsite, piwik_url);
        --></script>
        <object><noscript><p><img src="http://sourceforge.net/apps/piwik/bloodfrontier/piwik.php?idsite=1" alt="piwik"/></p></noscript></object>
    </head>
    <body style="background: #111111 url(<?php echo $app['background']; ?>) no-repeat center top">
        <div id="container">
            <div id="links"><?php echo $app['navbar']; ?></div>
            <div id="header">
                <a href="/home"><img src="/bits/lightbox-blank.gif" alt="Blood Frontier" width="450" height="143" border="0" align="left" title="Blood Frontier" /></a><a href="http://www.cubeengine.com/"><img src="/bits/lightbox-blank.gif" alt="Built on Cube Engine 2" width="150" height="143" border="0" align="right" title="Built on Cube Engine 2" /></a>
            </div>
            <div id="video">
                <div id="main">
                    <h1>Blood Frontier</h1>
                    <h2><i>It's Bloody Fun!</i></h2>
                    <h3>Free and open source game for Windows, Linux/BSD and Mac OSX</h3>
                    <h3>Agile gameplay in low gravity environments: sprint, wall run/kick, and impulse dash</h3>
                    <h3>Create your own maps cooperatively with friends online using the builtin map editor</h3>
                    <a href="/download" id="button">Free Download <em><?php echo $app['releasever']; ?></em></a> 
                    <p id="rel">released <i><?php echo $app['releasedate']; ?></i></p>
                    <p id="svn">or <a href="/wiki/SVN">get the SVN version</a> and live on the bleeding edge</p>
                </div>
                <div id="player">
                    <object width="500" height="308" type="application/x-shockwave-flash" data="http://www.youtube.com/v/uKnLsAiCVLk&amp;color1=0x000000&amp;color2=0x000000&amp;border=0&amp;fs=1&amp;egm=0&amp;showsearch=0&amp;showinfo=0&amp;ap=%2526fmt%3D18">
                        <param name="movie" value="http://www.youtube.com/v/uKnLsAiCVLk&amp;color1=0x000000&amp;color2=0x000000&amp;border=0&amp;fs=1&amp;egm=0&amp;showsearch=0&amp;showinfo=0&amp;ap=%2526fmt%3D18" />
                        <param name="allowscriptaccess" value="always" />
                        <param name="allowFullScreen" value="true" />
                    </object>
                </div>
            </div>
            <div class="endblock">&nbsp;</div>
            <div class="sliderblock">
                <div id="slider">
                    <ul id="gallery">               
                        <li>
                            <a href="/bits/images/screenshot.0006.jpg"><img src="/bits/thumbs/screenshot.0006.jpg" width="220" height="140" border="0" alt="Screenshot 1" /></a>
                            <a href="/bits/images/screenshot.0008.jpg"><img src="/bits/thumbs/screenshot.0008.jpg" width="220" height="140" border="0" alt="Screenshot 2" /></a>
                            <a href="/bits/images/screenshot.0010.jpg"><img src="/bits/thumbs/screenshot.0010.jpg" width="220" height="140" border="0" alt="Screenshot 3" /></a>
                            <a href="/bits/images/screenshot.0020.jpg"><img src="/bits/thumbs/screenshot.0020.jpg" width="220" height="140" border="0" alt="Screenshot 4" /></a>
                        </li>
                        <li>
                            <a href="/bits/images/screenshot.0013.jpg"><img src="/bits/thumbs/screenshot.0013.jpg" width="220" height="140" border="0" alt="Screenshot 5" /></a>
                            <a href="/bits/images/screenshot.0033.jpg"><img src="/bits/thumbs/screenshot.0033.jpg" width="220" height="140" border="0" alt="Screenshot 6" /></a>
                            <a href="/bits/images/screenshot.0036.jpg"><img src="/bits/thumbs/screenshot.0036.jpg" width="220" height="140" border="0" alt="Screenshot 7" /></a>
                            <a href="/bits/images/screenshot.0038.jpg"><img src="/bits/thumbs/screenshot.0038.jpg" width="220" height="140" border="0" alt="Screenshot 8" /></a>
                        </li>
                        <li>
                            <a href="/bits/images/screenshot.0034.jpg"><img src="/bits/thumbs/screenshot.0034.jpg" width="220" height="140" border="0" alt="Screenshot 9" /></a>
                            <a href="/bits/images/screenshot.0008.jpg"><img src="/bits/thumbs/screenshot.0008.jpg" width="220" height="140" border="0" alt="Screenshot 10" /></a>
                            <a href="/bits/images/screenshot.0010.jpg"><img src="/bits/thumbs/screenshot.0010.jpg" width="220" height="140" border="0" alt="Screenshot 11" /></a>
                            <a href="/bits/images/screenshot.0020.jpg"><img src="/bits/thumbs/screenshot.0020.jpg" width="220" height="140" border="0" alt="Screenshot 12" /></a>
                        </li>
                    </ul>
                </div>
            </div>
            <div class="endblock">&nbsp;</div>
            <div class="leftcol">
                <h4>About the Project</h4>
                <p>Blood Frontier is a <a href="/wiki/Collaborate">Free and Open Source</a> game (see our <a href="/wiki/License">License</a>), using <a href="http://libsdl.org/">SDL</a> and <a href="http://opengl.org/">OpenGL</a> which allows it to be ported to many platforms; you can <a href="/download">download a package</a> for Windows, Linux/BSD, Mac OSX, or grab a development copy from our <a href="/wiki/SVN">Subversion</a> repository and live on the bleeding edge. If you\'re an Ubuntu user, you might like to use the <a href="/playdeb">easy-to-install package available on PlayDeb.net</a>.</p>
                <p>The game is a single-player and multi-player first-person ego-shooter, built as a total conversion of <a href="http://www.cubeengine.com/">Cube Engine 2</a>, which lends itself toward a balanced gameplay, completely at the control of map makers, while maintaining a general theme of agility in a low gravity environment. For more information, please see our <a href="/wiki">Wiki</a>.</p>
                <p>In a true open source <i>by the people for the people</i> nature, it tries to work closely with the gaming and open source communities to provide a better overall experience, taking all sorts of feedback from your average player, to your seasoned developer, aiming to create a story-driven game environment that is flexible, fun, easy to use, and pleasing to the eye.</p>
            </div>
            <div class="vbar">&nbsp;</div>
            <div class="leftcol">
                <h4>Synopsis</h4>
                <p>In the distant future, humanity has spread throughout the universe, to ends of the galaxy and beyond. A vast communications network bridges from colony to colony, human to machine, and machine to human. This seemingly benign keystone of modern inter-planetary society, however, appears to be the carrier of a mysterious techno-biological plague. Any persons so-connected seem to fall ill and die, only to return as ravenous, sub-human cannibals.You, a machine intelligence, an android, remain unafflicted by this strange phenomenon and have been tasked with destroying the growing hordes of the infected, while, hopefully, locating and stopping the source of the epidemic.</p>
            </div>
            <div class="rightblock">
                <h4>Support Us</h4>
                <p id="donatemsg">Blood Frontier is developed by volunteers, and you get it free of charge; your donations go toward ongoing costs which keep this project alive.</p>
                <form action="https://www.paypal.com/cgi-bin/webscr" method="post">
                    <input type="hidden" name="cmd" value="_s-xclick" />
                    <input type="hidden" name="hosted_button_id" value="212900" />
                    <input type="image" src="https://www.paypal.com/en_AU/i/btn/btn_donate_LG.gif" name="submit" alt="Support Us - Donate" />
                    <img alt="Support Us - Donate" border="0" src="https://www.paypal.com/en_AU/i/scr/pixel.gif" width="1" height="1" />
                </form>
            </div>
            <div class="endrightblock">&nbsp;</div>
            <div class="rightblock">
                <ul>
<?php               foreach ($app['targets'] as $key => $targ) {
                        if ($key != "" && $targ['name'] != "" && $targ['nav'] == 0) {
                            echo "<li><a href=\"/". $key ."\" class='info'><span id=\"". $targ['name'] ."\">&nbsp;</span><span class=\"bubble\">on ". $targ['name'] ."</span></a></li>";
                        }
                    } ?>
                </ul>
            </div>
            <div class="endrightblock">&nbsp;</div>
            <div id="footer">
                <div id="sflogo">
                    <a href="/project">
                        <img src="http://sflogo.sourceforge.net/sflogo.php?group_id=198419&amp;type=11" alt="Get Blood Frontier at SourceForge" title="Get Blood Frontier at SourceForge" width="120" height="30" border="0" />
                    </a>
                </div>
                <a href="/download">Download</a>, <a href="/wiki">Learn More</a>, or <a href="/chat">Get Help</a> today.
            </div>
            <div id="copyright" align="center">
                <p>Blood Frontier, Copyright (C) 2006-2010 Anthony Cord, Quinton Reeves, Lee Salzman</p>
                <p>Cube Engine 2, Copyright (C) 2001-2010 Wouter van Oortmerssen, Lee Salzman, Mike Dysart, Robert Pointon, and Quinton Reeves</p>
            </div>
        </div>
    </body>
</html>
<?php } ?>
