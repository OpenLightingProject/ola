<html>
 <head>
 <title>OLAD - Universe RDM Info</title>
 <link rel="stylesheet" type="text/css" href="/simple.css"/>
</head>

 <body>

  <span class="heading2">RDM UIDS for Universe {{UNIVERSE:h}}</span>

 <div align="center" style="margin-top: 40px">
   <table width="300px" border="0" cellspacing="1" cellpadding="0" class="info">
    <tr class="info_head">
     <th>UIDS</th>
    </tr>
    {{#NO_UIDS}}
     <tr><td align="center">No uids found</td></tr>
    {{/NO_UIDS}}
    {{#UID}}
     <tr {{#ODD}}class="odd"{{/ODD}}>
      <td align="center">{{UID:h}}</td>
     </tr>
    {{/UID}}
   </table>
   <br/>
   <form action="/rdm" method="get">
     <input type="submit" name="action" value="Force Discovery"/>
     <input type="hidden" name="universe" value="{{UNIVERSE:h}}"/>
   </form>
  </div>
 </body>
</html>
