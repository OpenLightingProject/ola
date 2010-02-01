<html>
<head>
 <title>OLAD - Info</title>
 <link rel="stylesheet" type="text/css" href="/simple.css"/>

<script language="JavaScript" type="text/javascript">

<!--

function toggleL(event,id) {
    var menu = document.getElementById('dev_' + id);
    var img = document.getElementById('img_' + id);
    var f = document.getElementById('show_' + id);
    var display = menu.style.display ;
    if (display == "block") {
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

function setupPriorityBox(port_id, value) {
  var select = document.getElementById(port_id + '_priority_mode');
  for (var i = 0; i < select.length; ++i) {
    if (select.options[i].value == value) {
      select.options[i].selected = true;
    }
  }
}

function priorityBoxChanged(port_id) {
  var select = document.getElementById(port_id + '_priority_mode');
  var input = document.getElementById(port_id + '_priority_value');
  if (select.selectedIndex) {
    input.style.display = 'inline';
  } else {
    input.style.display = 'none';
  }
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
      <td width="40px" align="center">{{CAPABILITY:h}}</td>
      <td width="60px"><input type="text" name="{{PORT_ID:h}}" size="4" value="{{UNIVERSE:h}}"/></td>
      <td>{{DESCRIPTION:h}}</td>
      <td>
        {{#SUPPORTS_PRIORITY}}
         Priority:
          {{#SUPPORTS_PRIORITY_MODE}}
            <select name="{{PORT_ID:h}}_priority_mode"
                    id="{{PORT_ID:h}}_priority_mode"
                    onChange="priorityBoxChanged('{{PORT_ID:j}}')">
              <option value="0">Inherit</option>
              <option value="1">Override</option>
            </select>
            <script type="text/javascript">
              setupPriorityBox('{{PORT_ID:j}}', {{PRIORITY_MODE:j}});
            </script>
          {{/SUPPORTS_PRIORITY_MODE}}
         <input type="text" id="{{PORT_ID:h}}_priority_value"
                name="{{PORT_ID:h}}_priority_value" size="4"
                value="{{PRIORITY:h}}">
          {{#SUPPORTS_PRIORITY_MODE}}
            <script type="text/javascript">priorityBoxChanged('{{PORT_ID:j}}');</script>
          {{/SUPPORTS_PRIORITY_MODE}}
        {{/SUPPORTS_PRIORITY}}
      </td>
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
