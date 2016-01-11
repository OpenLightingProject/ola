wwwdir = $(www_datadir)
newdir = $(www_datadir)/new
viewsdir = $(www_datadir)/new/views
jsdir = $(www_datadir)/new/js
cssdir = $(www_datadir)/new/css
imgdir = $(www_datadir)/new/img
jquerydir = $(www_datadir)/new/libs/jquery/js
angularroutedir = $(www_datadir)/new/libs/angular-route/js
angulardir = $(www_datadir)/new/libs/angular/js
bootcssdir = $(www_datadir)/new/libs/bootstrap/css
bootjsdir = $(www_datadir)/new/libs/bootstrap/js
bootfontsdir = $(www_datadir)/new/libs/bootstrap/fonts

dist_www_DATA = \
    olad/www/back.png \
    olad/www/blank.gif \
    olad/www/button-bg.png \
    olad/www/console_values.html \
    olad/www/custombutton.css \
    olad/www/discovery.png \
    olad/www/editortoolbar.png \
    olad/www/expander.png \
    olad/www/forward.png \
    olad/www/handle.vertical.png \
    olad/www/hide_sections.png \
    olad/www/incremental-discovery.png \
    olad/www/landing.html \
    olad/www/light_bulb_off.png \
    olad/www/light_bulb.png \
    olad/www/loader-mini.gif \
    olad/www/loader.gif \
    olad/www/logo-mini.png \
    olad/www/logo.png \
    olad/www/mobile.html \
    olad/www/mobile.js \
    olad/www/ola.html \
    olad/www/ola.js \
    olad/www/refresh.png \
    olad/www/show_sections.png \
    olad/www/tick.gif \
    olad/www/toolbar-bg.png \
    olad/www/toolbar.css \
    olad/www/toolbar_sprites.png \
    olad/www/vertical.gif \
    olad/www/wand.png \
    olad/www/warning.png
dist_new_DATA = \
    olad/www/new/index.html
dist_views_DATA = \
    olad/www/new/views/overview.html \
    olad/www/new/views/plugin-info.html \
    olad/www/new/views/plugins.html \
    olad/www/new/views/universe-add.html \
    olad/www/new/views/universe-faders.html \
    olad/www/new/views/universe-header.html \
    olad/www/new/views/universe-keypad.html \
    olad/www/new/views/universe-overview.html \
    olad/www/new/views/universe-patch.html \
    olad/www/new/views/universe-rdm.html \
    olad/www/new/views/universe-settings.html \
    olad/www/new/views/universes.html
dist_js_DATA = \
    olad/www/new/js/app.min.js \
    olad/www/new/js/app.min.js.map
dist_css_DATA = \
    olad/www/new/css/style.min.css
dist_img_DATA = \
    olad/www/new/img/light_bulb_off.png \
    olad/www/new/img/light_bulb.png \
    olad/www/new/img/logo-mini.png \
    olad/www/new/img/logo-mini@2x.png \
    olad/www/new/img/logo.png \
    olad/www/new/img/favicons/android-chrome-144x144.png \
    olad/www/new/img/favicons/android-chrome-192x192.png \
    olad/www/new/img/favicons/android-chrome-36x36.png \
    olad/www/new/img/favicons/android-chrome-48x48.png \
    olad/www/new/img/favicons/android-chrome-72x72.png \
    olad/www/new/img/favicons/android-chrome-96x96.png \
    olad/www/new/img/favicons/apple-touch-icon-114x114.png \
    olad/www/new/img/favicons/apple-touch-icon-120x120.png \
    olad/www/new/img/favicons/apple-touch-icon-144x144.png \
    olad/www/new/img/favicons/apple-touch-icon-152x152.png \
    olad/www/new/img/favicons/apple-touch-icon-180x180.png \
    olad/www/new/img/favicons/apple-touch-icon-57x57.png \
    olad/www/new/img/favicons/apple-touch-icon-60x60.png \
    olad/www/new/img/favicons/apple-touch-icon-72x72.png \
    olad/www/new/img/favicons/apple-touch-icon-76x76.png \
    olad/www/new/img/favicons/apple-touch-icon-precomposed.png \
    olad/www/new/img/favicons/apple-touch-icon.png \
    olad/www/new/img/favicons/apple-touch-startup-image-1182x2208.png \
    olad/www/new/img/favicons/apple-touch-startup-image-1242x2148.png \
    olad/www/new/img/favicons/apple-touch-startup-image-1496x2048.png \
    olad/www/new/img/favicons/apple-touch-startup-image-1536x2008.png \
    olad/www/new/img/favicons/apple-touch-startup-image-320x460.png \
    olad/www/new/img/favicons/apple-touch-startup-image-640x1096.png \
    olad/www/new/img/favicons/apple-touch-startup-image-640x920.png \
    olad/www/new/img/favicons/apple-touch-startup-image-748x1024.png \
    olad/www/new/img/favicons/apple-touch-startup-image-750x1294.png \
    olad/www/new/img/favicons/apple-touch-startup-image-768x1004.png \
    olad/www/new/img/favicons/browserconfig.xml \
    olad/www/new/img/favicons/coast-228x228.png \
    olad/www/new/img/favicons/favicon-16x16.png \
    olad/www/new/img/favicons/favicon-230x230.png \
    olad/www/new/img/favicons/favicon-32x32.png \
    olad/www/new/img/favicons/favicon-96x96.png \
    olad/www/new/img/favicons/favicon.ico \
    olad/www/new/img/favicons/firefox_app_128x128.png \
    olad/www/new/img/favicons/firefox_app_512x512.png \
    olad/www/new/img/favicons/firefox_app_60x60.png \
    olad/www/new/img/favicons/manifest.json \
    olad/www/new/img/favicons/manifest.webapp \
    olad/www/new/img/favicons/mstile-144x144.png \
    olad/www/new/img/favicons/mstile-150x150.png \
    olad/www/new/img/favicons/mstile-310x150.png \
    olad/www/new/img/favicons/mstile-310x310.png \
    olad/www/new/img/favicons/mstile-70x70.png \
    olad/www/new/img/favicons/open-graph.png \
    olad/www/new/img/favicons/twitter.png \
    olad/www/new/img/favicons/yandex-browser-50x50.png \
    olad/www/new/img/favicons/yandex-browser-manifest.json
dist_jquery_DATA = \
    olad/www/new/libs/jquery/js/jquery.min.js
dist_angularroute_DATA = \
    olad/www/new/libs/angular-route/js/angular-route.min.js
dist_angular_DATA = \
    olad/www/new/libs/angular/js/angular.min.js
dist_bootjs_DATA = \
    olad/www/new/libs/bootstrap/js/bootstrap.min.js
dist_bootfonts_DATA = \
    olad/www/new/libs/bootstrap/fonts/glyphicons-halflings-regular.eot \
    olad/www/new/libs/bootstrap/fonts/glyphicons-halflings-regular.svg \
    olad/www/new/libs/bootstrap/fonts/glyphicons-halflings-regular.ttf \
    olad/www/new/libs/bootstrap/fonts/glyphicons-halflings-regular.woff \
    olad/www/new/libs/bootstrap/fonts/glyphicons-halflings-regular.woff2
dist_bootcss_DATA = \
    olad/www/new/libs/bootstrap/css/bootstrap.min.css
