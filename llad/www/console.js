var MAX_VAL = 255;  // dmx max value
var MAX_CHAN = 512;  // total channels
var MAX_MEM = 4;    // memory locations
var CHAN = 16;       // channels per page
var offset = 0;     // starting page offset
var b = Array(MAX_CHAN);  // dmx buffer
var ss = Array(CHAN);     // list of sliders
var m = Array(MAX_MEM) ;  // memory
var ls = ''  ;            // last values sent (as str)
var t;
var es = Array(MAX_CHAN) ;

function onLoad() {
 for (var i=0; i<CHAN; ++i) {
  document.forms['vs'].elements['v'+i].value=0;
  document.forms['vs'].elements['c'+i].value=i+1;
 }
 var vfm = window.frames['vs'].document.forms['v'] ;

 for (var i =0 ; i<MAX_CHAN; ++i) {
  es[i] = vfm.elements['l' + i];
 }

 mset(b,0);
 sv_all();

 for (var i=0; i<MAX_MEM; ++i) {
  m[i] = new Array(MAX_CHAN);
  mset(m[i], 0);
 }

 t = setTimeout("snd_all()", 500);
}

function mset(m, v) {
 for(var i=0; i<MAX_CHAN; ++i) {
  m[i] = v;
 }
}

function copy(s, d) {
 for (var i=0; i<MAX_CHAN; ++i) {
  d[i] = s[i];
 }
}

function has_c() {
   return (ls == b.join() ? 0 : 1);
}


function sv_all() {
 for(var i=0; i<MAX_CHAN; ++i) {
  sv(i, b[i]) ;
 }
}


function sv(c,v) {
  var e = es[c];
  if(e) {
    e.value=v ;
    var i = MAX_VAL-v;
   e.style.background= "rgb("+i+','+i+','+i+')';
    if( v>90) {
      e.style.color = "#ffffff";
    } else {
     e.style.color = "#000000";
    }
  }
}


function snd_all() {
  return;
  if (has_c()) {
    var bs = b.join();
    ls = bs;
    AjaxRequest.get ( {
      'url':'/set_dmx',
      'method':'POST',
      'parameters':{ 'd': bs , 'u': u },
      'onSuccess':function(req) { snd_all() },
      'onError':function(req){ alert('Error!\nStatusText='+req.statusText+'\nContents='+req.responseText);},
      'timeout':2000,
      'onTimeout':function(req){ alert('Timed Out!'); }
    });
  } else {
    t = setTimeout("snd_all()", 300);
  }
}

function set(c,v) {
  var ch = c+offset;
  var e2 = document.forms['vs'].elements['v'+c];
  e2.value=v;
  b[ch] = v;
  sv(c+offset, v);
}

function up_chans() {
 for (var i=0; i<CHAN; ++i) {
  document.forms['vs'].elements['c'+i].value=i+offset+1;
 }
}

function up_vals() {
 for (var i=0; i<CHAN; ++i) {
   ss[i].setValue(b[i+offset]);
 }
}

function next() {
  if (offset < MAX_CHAN - CHAN -1) {
    offset += CHAN;
    up_chans();
    up_vals();
  }
}

function back() {
  if (offset != 0) {
    offset -= CHAN;
    up_chans();
    up_vals();
  }
}

function full() {
  mset(b, MAX_VAL);
  sv_all();
  up_vals();
}

function dbo() {
  mset(b,0);
  sv_all();
  up_vals()
}

function save() {
  var n = document.getElementById('m').value;
  if (n >= 0 &&  n < MAX_MEM) {
    copy(b,m[n]);
  }
}

function load(n) {
 var n = document.getElementById('m').value;
 if (n >= 0 &&  n < MAX_MEM) {
  copy(m[n],b)
 }
 up_vals();
 sv_all();
}

