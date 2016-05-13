/**
 * @file
 * Better Comments provides option to configure the comment system.
 */

(function ($) {
  Drupal.behaviors.betterComments = {
    attach: function(context, settings) {
      $("a.better-comments-reply", context).click(function() {
        replyid = $(this).attr("id").split('-');
        $(this).hide('slow');
      });
      $("a.reply-cancel, a.preview-cancel", context).click(function() {
        cancelid = $(this).attr("id").split('-');
        $('#reply-' + cancelid[1]).show('slow');
      });
      $("a.better-comments-delete", context).click(function() {
        deleteid = $(this).attr("id").split('-');
        $(this).hide('slow');
      });
      $("a.delete-cancel", context).click(function() {
        cancelid = $(this).attr("id").split('-');
        $('#delete-' + cancelid[2]).show('slow');
      });
      if($('.better-comments-reply').is(":hidden")) {
        $.each($('.better-comments-reply', context), function() {
          child = $(this).closest(".better-comment-wrapper");
          parent = $(child).parents(".better-comment-wrapper").attr("id").split('-');
          if(($("#" + parent + " .comment-form").length > 0)) {
          }
          else {
            $('#reply-' + parent[2]).show('slow');
          }
        });
      }
    }
  }
}(jQuery));
