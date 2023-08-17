-- 创建users表
CREATE TABLE
  IF NOT EXISTS users (
    id INT NOT NULL AUTO_INCREMENT,
    name VARCHAR(255) NOT NULL,
    password VARCHAR(255) NOT NULL,
    email VARCHAR(255) NOT NULL,
    PRIMARY KEY (id)
  );


-- 插入100条随机数据
DELIMITER / / DROP PROCEDURE IF EXISTS generate_random_data / / CREATE PROCEDURE generate_random_data() BEGIN DECLARE i INT DEFAULT 1;


WHILE i <= 100 DO
INSERT INTO
  users (name, password, email)
VALUES
  (
    CONCAT('User', i),
    CONCAT('password', i),
    CONCAT('user', i, '@example.com')
  );


SET
  i = i + 1;


END WHILE;


END / / CALL generate_random_data() / / DROP PROCEDURE IF EXISTS generate_random_data DELIMITER;


SELECT
  chathistory.speaker_id,
  chathistory.speaker_name,
  chathistory.chatid,
  chathistory.message,
  chathistory.messagetime
FROM
  chathistory
  JOIN (
    SELECT
      chatid
    FROM
      relationship
    WHERE
      tgt1 = 1
  ) AS relation ON relation.chatid = chathistory.chatid
WHERE
  chathistory.messagetime > '2000-02-02'
ORDER BY
  chathistory.speaker_id,
  chathistory.messagetime;


SELECT
  mode,
  data,
  time
FROM
  notify
WHERE
  receiver_id = id
ORDER BY
  time -- select relationship.tgt1, relationship.stat , relationship.chatid FROM relationship , user_set where relationship.tgt2 = 'id' and relationship.chatid != user_set.chatid 
  -- select relationship.tgt2 , users.name , usres.email , relationship.stat , relationship.chatid FROM relationship where relationship.tgt1 = 'id' and users.id = relationship.tgt2  ORDER BY users.name;
  -- 查朋友关系
SELECT
  relationship.tgt2,
  users.name,
  users.email,
  relationship.stat,
  relationship.chatid
FROM
  relationship
  INNER JOIN users ON users.id = relationship.tgt2
WHERE
  relationship.tgt1 = 1
ORDER BY
  users.name;


--------------- 查群组关系
select
  user_set.user_set_id,
  user_set.name,
  relationship.stat,
  users.id,
  users.name,
  user_set.chatid
FROM
  users,
  relationship,
  user_set,
  (
    select
      tgt1
    from
      relationship
    where
      tgt2 = 1
  ) AS group_ids
where
  user_set.user_set_id = group_ids.tgt1
  and relationship.tgt1 = group_ids.tgt1
  and users.id = relationship.tgt2
ORDER BY
  user_set.user_set_id,
  relationship.stat,
  users.name ------------------ 
SELECT
  us.user_set_id,
  us.name,
  r.stat,
  u.id,
  u.name,
  us.chatid
FROM
  users AS u
  INNER JOIN (
    SELECT
      tgt1
    FROM
      relationship
    WHERE
      tgt2 = 1
  ) AS group_ids
  INNER JOIN relationship AS r ON r.tgt1 = group_ids.tgt1
  INNER JOIN user_set AS us ON us.user_set_id = group_ids.tgt1
WHERE
  u.id = r.tgt2
ORDER BY
  us.user_set_id,
  r.stat,
  u.name;


-------------------
SELECT
  us.user_set_id,
  us.name,
  r.stat,
  u.id,
  u.name,
  us.chatid
FROM
  users AS u
  INNER JOIN relationship AS r ON u.id = r.tgt2
  INNER JOIN user_set AS us ON us.user_set_id = r.tgt1
  INNER JOIN (
    SELECT
      tgt1
    FROM
      relationship
    WHERE
      tgt2 = 1000000001
  ) AS group_ids ON us.user_set_id = group_ids.tgt1
ORDER BY
  us.user_set_id,
  r.stat,
  u.name;


----------------------------------------------------------------
select
  online_users.id,
  online_users.name
from
  online_users,
  (
    select
      tgt2
    FROM
      relationship
    where
      tgt1 = id
  ) as friend_ids
WHERE
  online_users.id = friend_ids.tgt2
ORDER BY
  online_users.name




ALTER TABLE relationship 
MODIFY tgt1 BIGINT UNSIGNED NOT NULL,
MODIFY tgt2 BIGINT UNSIGNED NOT NULL,
MODIFY chatid varchar(255) NOT NULL;
