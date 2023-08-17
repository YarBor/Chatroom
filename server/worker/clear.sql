delete from
  chathistory;


delete from
  notify;


delete from
  online_users;


delete from
  relationship;


delete from
  user_set;


delete from
  users;



ALTER TABLE user_set MODIFY COLUMN user_set_id INT UNSIGNED NOT NULL AUTO_INCREMENT FIRST, AUTO_INCREMENT=0;
ALTER TABLE users MODIFY COLUMN id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT FIRST, AUTO_INCREMENT=0;
