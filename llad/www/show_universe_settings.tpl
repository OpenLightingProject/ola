<html>
 <head>
 <title>LLAD - Universe Info</title>
 <link rel="stylesheet" type="text/css" href="/simple.css"/>
 <script>
 <!--

 function openConsole(u) {
   window.open('/cgi-bin/console.pl?u=' + u, 'console', "width=730,height=320");
 }
 -->
 </script>
</head>

 <body>

  <span class="heading2">Universe Info</span>
 <form action="/universes" method="get">

 <div align="center" style="margin-top: 40px">
   <table border="0" cellspacing="1" cellpadding="0" class="info">
    <tr class="info_head">
     <th>Universe</th>
     <th>Name</th>
     <th>Merge Mode</th>
     <th>Console</th>
    </tr>
    {{#NO_UNIVERSES}}
     <tr><td colspan="4" align="center">No universes active</td></tr>
    {{/NO_UNIVERSES}}
    {{#UNIVERSE}}
     <tr {{#ODD}}class="odd"{{/ODD}}>
      <td align="center">{{ID:h}}</td>
      <td><input type="text" name="name_{{ID:h}}" value="{{NAME}}"/></td>
      <td align="center">
        <select name="mode_{{ID:h}}">
         <option value="ltp">LTP</option>
         <option value="htp" {{#HTP_MODE}}selected{{/HTP_MODE}}>HTP</option>
        </select></td>
      <td align="center"><a href="#" onClick="openConsole({{ID:h}})">Console</a></td>
     </tr>
     {{/UNIVERSE}}
   </table>
   <br/>
  <input type="submit" name="action" value="Apply Changes"/>
  </form>
  </div>
 </body>
</html>
