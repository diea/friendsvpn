<ul class="nav nav-pills nav-stacked">
  <?php
if ($services) {
  foreach($services as $service) {
      echo "<li class=\"liservice\">";
      ?>
      <a href="#" data-name="<?php echo $service["name"]; ?>" class="serviceli"><?php echo $service["fancy_name"]; ?></a>
      <?php
      echo "</li>";
  }
  ?>
  <br>
  <button type="button" id ="deleteAll" class="btn btn-danger">Clean up all records</button>
  <?php
}
  ?>
</ul>

<script>
    /* ajax call for the services offered by a given host */
    $(".serviceli").click(function(event) {
        event.preventDefault();
        //alert($(this).html());
        $(".liservice").removeClass("active");
        $(this).parent().addClass("active");
        
        // remove before reloading necessary things
        $("#serviceNamePortList").html("");
        $("#hostsList").html("");
        $(".permissionsField").html("")
        $(".specialProtocolFeatures").html("");
        
        var service = $(this).data("name");
        $.ajax({
            type: "POST",
            url:"bonjourgui/hostnames/",
            data: {
                "service": service
            }
        }).done(function(data) {
            $("#hostsList").html(data);
        });
    });
    
$("#deleteAll").click(function(event) {
    event.preventDefault();
    // deauthorize
    $.ajax({
        url: "/bonjourGui/deleteAll",
        type: "GET",
    }).done(function() {
        // reload page
        location.reload(); // refresh page
    });
});
</script>