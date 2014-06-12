<ul class="nav nav-pills nav-stacked">
  <?php
  if ($hostnames) {
      foreach($hostnames as $hostname) {
          echo "<li class=\"lihost\">";
          ?>
          <a href="#" class="hostnameli"><?php echo $hostname["hostname"]; ?></a>
          <?php
          echo "</li>";
      }
  } else {
      echo "<p>ERROR: No hostname found</p>";
  }

  ?>
</ul>

<script>
    /* ajax call for the services offered by a given host */
    $(".hostnameli").click(function(event) {
        event.preventDefault();
        //alert($(this).html());
        $(".lihost").removeClass("active");
        $(this).parent().addClass("active");
        var hostname = $(this).html();
        $.ajax({
            type: "POST",
            url:"bonjourgui/serviceNamePortList/",
            data: {
                "hostname": hostname,
                "service": $(".active.liservice").children().first().data("name")
            }
        }).done(function(data) {
            $("#serviceNamePortList").html(data);
        });
    });
</script>