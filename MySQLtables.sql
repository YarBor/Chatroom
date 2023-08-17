-- 这个表存储用户数据，包括他们的ID、姓名和密码。
CREATE TABLE IF NOT EXISTS users (
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL, -- 用户的名字
    password VARCHAR(255) NOT NULL, -- 用户的密码
    email VARCHAR(255) NOT NULL DEFAULT "************"
);


CREATE TABLE IF NOT EXISTS relationship (
    tgt1 INT NOT NULL, -- 主 ID    
    tgt2 INT NOT NULL, -- 关系 ID
    stat INT NOT NULL DEFAULT 1, -- 加好友 拉黑用 表示朋友状态
    chatid VARCHAR(255) NOT NULL -- 朋友关系的聊天ID
);

CREATE TABLE IF NOT EXISTS user_set (
    user_set_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, -- 聊天室的索引
    name VARCHAR(255) NOT NULL, -- 聊天室的名称
    own_name VARCHAR(255) NOT NULL,  -- 聊天室所有者的名字
    owner_id INT NOT NULL,  -- 聊天室的所有者ID
    chatid VARCHAR(255) NOT NULL -- 聊天室的唯一标识符
);

CREATE TABLE IF NOT EXISTS chathistory (
    speaker_id INT NOT NULL , 
    speaker_name VARCHAR(255) NOT NULL ,
    chatid VARCHAR(255) NOT NULL , -- 聊天记录的唯一标识符
    message TEXT NOT NULL, -- 聊天记录的消息内容
    messagetime DATETIME DEFAULT CURRENT_TIMESTAMP -- 发送时间（默认为当前时间戳）
);

CREATE TABLE IF NOT EXISTS notify ( 
    receiver_id INT NOT NULL, 
    mode INT NOT NULL, 
    data text NOT NULL , 
    time datetime NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS online_users (
    id INT NOT NULL, 
    name VARCHAR(255) NOT NULL
);

