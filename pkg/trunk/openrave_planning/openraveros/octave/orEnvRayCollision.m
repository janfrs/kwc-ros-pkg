% [collision, colinfo, hitbodies] = orEnvRayCollision(rays,[bodyid],req_contacts,req_bodyinfo)
%
% performs ray collision checks and returns the position and normals
% where all the rays collide
% bodyid (optional) - if non zero, will only collide with that ray
% rays - a 6xN matrix where the first 3
% rows are the ray position and last 3 are the ray direction
% collision - N dim vector that is 1 for colliding rays and 0
% for non-colliding rays colinfo is a 6xN vector that describes 
% where the ray hit and the normal to the surface of the hit point
% where the first 3 columns are position and last 3 are normals
% if bodyid is specified, only checks collisions with that body

function [collision, colinfo, hitbodies] = orEnvRayCollision(rays,bodyid,req_contacts,req_bodyinfo)

session = openraveros_getglobalsession();
req = openraveros_env_raycollision();

numrays = size(rays,2);
req.rays = cell(numrays,1);
for i = 1:numrays
    req.rays{i}.position = {rays(1,i),rays(2,i),rays(3,i)};
    req.rays{i}.direction = {rays(4,i),rays(5,i),rays(6,i)};
end

if( exist('bodyid','var') )
    req.bodyid = bodyid;
end
if( exist('req_contacts','var') && req_contacts )
    req.request_contacts = 1;
end
if( exist('req_bodyinfo','var') && req_bodyinfo )
    req.request_bodies = req_bodyinfo;
end

res = rosoct_session_call(session.id,'env_raycollision',req);

if(~isempty(res))
    collision = cell2mat(res.collision);

    if( req.request_contacts )
        if( length(res.contacts) ~= numrays )
            error('not equal');
        end
        colinfo = zeros(6,numrays);
        for i = 1:numrays
            colinfo(1:3,i) = cell2mat(res.contacts{i}.position);
            colinfo(4:6,i) = cell2mat(res.contacts{i}.normal);
        end
    else
        colinfo = [];
    end

    hitbodies = cell2mat(res.hitbodies);
else
    collision = [];
    colinfo = [];
    hitbodies = [];
end
