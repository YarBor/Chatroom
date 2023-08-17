SET GLOBAL event_scheduler = ON;

CREATE DATABASE IF NOT EXISTS chatroom;
use DATABASE chatroom;

-- 这个表存储用户数据，包括他们的ID、姓名和密码。
CREATE TABLE IF NOT EXISTS users (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL, -- 用户的名字
    password VARCHAR(255) NOT NULL, -- 用户的密码
    email VARCHAR(255) NOT NULL 
);


CREATE TABLE IF NOT EXISTS relationship (
    tgt1 BIGINT UNSIGNED NOT NULL, -- 主 ID    
    tgt2 BIGINT UNSIGNED NOT NULL, -- 关系 ID
    stat INT NOT NULL DEFAULT 1, -- 加好友 拉黑用 表示朋友状态
    chatid VARCHAR(255) NOT NULL -- 朋友关系的聊天ID
);

CREATE TABLE IF NOT EXISTS user_set (
    user_set_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, -- 聊天室的索引
    name VARCHAR(255) NOT NULL, -- 聊天室的名称
    own_name VARCHAR(255) NOT NULL,  -- 聊天室所有者的名字
    owner_id BIGINT UNSIGNED NOT NULL,  -- 聊天室的所有者ID
    chatid VARCHAR(255) NOT NULL -- 聊天室的唯一标识符
);

CREATE TABLE IF NOT EXISTS chathistory (
    speaker_id BIGINT UNSIGNED NOT NULL , 
    speaker_name VARCHAR(255) NOT NULL ,
    chatid VARCHAR(255) NOT NULL , -- 聊天记录的唯一标识符
    message TEXT NOT NULL, -- 聊天记录的消息内容
    messagetime timestamp(6) DEFAULT CURRENT_TIMESTAMP(6) -- 发送时间（默认为当前时间戳）
);

CREATE TABLE IF NOT EXISTS notify ( 
    receiver_id BIGINT UNSIGNED NOT NULL, 
    mode INT NOT NULL, 
    data blob NOT NULL , 
    time timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
);

CREATE TABLE IF NOT EXISTS online_users (
    id BIGINT UNSIGNED NOT NULL, 
    name VARCHAR(255) NOT NULL
);

CREATE EVENT delete_old_data
ON SCHEDULE EVERY 1 DAY
STARTS CURRENT_TIMESTAMP + INTERVAL 1 DAY
DO
    BEGIN
        -- 删除超过3天的聊天记录
        DELETE FROM chathistory WHERE messagetime < NOW() - INTERVAL 3 DAY;
        
        -- 删除超过3天的通知记录
        DELETE FROM notify WHERE time < NOW() - INTERVAL 3 DAY;
    END;
