<html>
<head>
 <title>LLAD - Info</title>
 <link rel="stylesheet" type="text/css" href="/simple.css"/>

<script language="JavaScript" type="text/javascript">

<!--

function toggleL(event,id) {
    var menu = document.getElementById('dev_' + id);
    var img = document.getElementById('img_' + id);
    var f = document.getElementById('show_' + id);
    var display = menu.style.display ;
    if( display == "block") {
        menu.style.display = "none";
        img.src = "/plus.png";
        f.value = 0;
    } else {
        menu.style.display = "block"
        img.src = "/minus.png"
        f.value = 1;
    }
    event.cancelBubble=true;
    event.returnValue=false;
}

// -->
</script>

</head>

<body>

 <span class="heading2">Device Info</span>

 <form action="/devices" method="post">
 <div align="center" style="margin-top: 40px">
  <div class="devices">
   {{#DEVICE}}
    <div class="dev_header" onClick="toggleL(event, '{{ID:j}}')">
     <img src="/plus.png" id="img_{{ID:h}}" style="vertical-align: bottom"/>
     Device {{ID:h}} - {{NAME:h}}
     <input id="show_{{ID:h}}" type="hidden" name="show_{{ID:h}}" value="{{SHOW_VALUE:h}}" />
    </div>

   <div id="dev_{{ID:h}}" class="dev_list" style="display: {{SHOW:h}}">

    <table class="dev_tbl" cellspacing="0">
    {{#PORT}}
     <tr {{#ODD}}class="odd"{{/ODD}}>
      <td width="40px">{{PORT_ID:h}}</td>
      <td width="40px">{{CAPABILITY:h}}</td>
      <td width="60px"><input type="text" name="{{ID:h}}_{{PORT_ID:h}}" size="4" value="{{UNIVERSE:h}}"/></td>
      <td>{{DESCRIPTION:h}}</td>
     </tr>
    {{/PORT}}
     </table>
   </div>
  {{/DEVICE}}
  {{#NO_DEVICES}}
    <div class="error">No devices!</div>
  {{/NO_DEVICES}}
 </div>

 <br/>
 <input type="submit" name="action" value="Save Changes"/>
</div>

 </form>
</body>
</html>
