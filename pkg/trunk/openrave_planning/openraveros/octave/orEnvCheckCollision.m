% [collision, colbodyid,contacts, mindist] = orEnvCheckCollision(bodyid,excludeid,req_contacts)
%
% Check collision of the robot with the environment. collision is 1 if the robot
% is colliding, colbodyid is the id of the object that body collided with
%% bodyid - the uid of the body, if size > 1, bodyidD(2) narrows collision down to specific body link (one-indexed)

function [collision, colbodyid, contacts, mindist] = orEnvCheckCollision(bodyid,excludeids,req_contacts)

session = openraveros_getglobalsession();
req = openraveros_env_checkcollision();

req.bodyid = bodyid(1);
if( length(bodyid)>1) 
    req.linkid = bodyid(2);
else
    req.linkid = -1; % all links of the body
end

if( ~exist('excludeid', 'var') )
    req.excludeids = mat2cell(excludeids,1,ones(length(excludeids),1));
end
if( exist('req_contacts','var') && req_contacts )
    req.options = req.options + req.CO_Contacts();
end

res = rosoct_session_call(session.id,'env_checkcollision',req);

if(~isempty(res))
    collision = res.collision;
    colbodyid = res.collidingbodyid;
    
    if( ~isempty(res.contacts) )
        contacts = zeros(6,numrays);
        for i = 1:length(res.contacts)
            colinfo(1:3,i) = cell2mat(res.contacts{i}.position);
            colinfo(4:6,i) = cell2mat(res.contacts{i}.normal);
        end
    else
        contacts = [];
    end

    hitbodies = cell2mat(res.hitbodies);
else
    collision = [];
    colbodyid = [];
    contacts = [];
    mindist = [];
end
