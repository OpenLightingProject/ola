<html>
<head>
<title>DMX Console</title>
<style>

.vs { position: absolute; right: 0px; height: 290px; width: 240px; border: 1px solid #000000}
.c td {text-align: center}
.c input { font-size: 10px; text-align: center; border: 0px; width:23px; }
.v td {text-align: center}
.v input {border: 1px solid #cccccc; font-size: 10px; text-align: center; width:29px }
.u { font-size: 11px; vertical-align: top}
.m select { font-size: 11px; border: 1px solid #000000; vertical-align: top}
.m img {vertical-align: middle}

</style>

<script type="text/javascript" src="/range.js"></script>
<script type="text/javascript" src="/timer.js"></script>
<script type="text/javascript" src="/slider.js"></script>
<script type="text/javascript" src="/ajax_request.js"></script>
<script type="text/javascript" src="/console.js"></script>
<script>
var u = "{{UNIVERSE:j}}";
</script>

</head>

<link type="text/css" rel="StyleSheet" href="/bluecurve.css" />

<body onLoad="onLoad({{ID:j}})">

<iframe class="vs" src="/console_values.html" name="vs" id="vs"></iframe>

<div class="m">
<span class="u">Universe: {{NAME:h}}</span>
<input type="image" src="/back.png" onClick="back()" value="Back" title="Previous Page" />
<input type="image" src="/forward.png" onClick="next()" value="Next" title="Next Page" />
<input type="image" src="/full.png"  onClick="full()" value="Full" title="Set all channels to 100%"/>
<input type="image" src="/dbo.png" onClick="dbo()" value="DBO" title="Set all channels to 0%"/>
<span class="u">Memory:</span>
<select name="m" id="m" class="mem">
 <option value="0">1</option>
 <option value="1">2</option>
 <option value="2">3</option>
 <option value="3">4</option>
</select>
<input type="image" src="/save.png"  onClick="save()" value="Save" title="Save to memory"/>
<input type="image" src="/load.png" onClick="load()" value="Load" title="Load from memory"/>

</div>

<form id="vs">

<table border="0" cellspacing="0" cellpadding="0">
 <tr class="c">
  {{#SLIDERS}}
   <td><input type="text" id="c{{INDEX:h}}"></td>
  {{/SLIDERS}}
 </tr>
 <tr class="v">
  {{#SLIDERS}}
   <td><input type="text" name="v{{INDEX:h}}"></td>
  {{/SLIDERS}}
 </tr>
 <tr>
  {{#SLIDERS}}
  <td>
<div class="vertical dynamic-slider-control slider" id="s{{INDEX:h}}" tabindex="1">
<input class="slider-input" id="si{{INDEX:h}}"></div>
  </td>
  {{/SLIDERS}}
 </tr>
</table>

</form>

<script type="text/javascript">

for(var i=0; i< CHAN; i++) {
  var s = new Slider(document.getElementById("s"+i), document.getElementById("si"+i), "vertical", i);
  s.setMinimum(0);
  s.setMaximum(MAX_VAL);
  s.setValue(0);

  var o = i;
  s.onchange = function(se) {
    set(se.getId(), se.getValue()) ;
  };
  ss[i] = s;
}

</script>

</body>
</html>
