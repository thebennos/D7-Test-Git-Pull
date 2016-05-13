<?php
/**
 * @file
 * Default theme implementation for comments.
 * $user_picture - Holds value if better_comments_picture is set or not.
 */
?>
<div class="comment-inner-<?php print $comment->cid; ?> clearfix"
<?php  print $attributes; ?>>
  <div class="comment-body">
  <?php  if($picture_set == 1): ?> 
    <?php print $user_picture; ?>
<?php endif;?>
  <?php print render($title_preffix); ?>

  <div class="comment-content"<?php print $content_attributes; ?>>
    <?php
      // We hide the comments and links now so that we can render them later.
    hide($content['links']);
    // Print username.
    print $author;
    print render($content);?>

     <div class="comment-data"> 
   <span><?php print $created; ?></span>
   <span><?php print render($content['links']) ?></span>
 
    <?php if ($signature): ?>
    <div class="user-signature clearfix">
      <?php print $signature ?>
    </div>
    <?php endif; ?>
  </div>
   </div> 
  </div>
</div>
</div>
