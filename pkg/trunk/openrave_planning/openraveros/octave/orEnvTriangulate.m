% [tripoints, triindices] = orEnvTriangulate(inclusive, ids)
%
% Returns the triangulation of various objects in the scenes controlled by name and options
% Arguments:
%   inclusive - if 1, will only triangulate the bodies pointed to by ids
%               if 0, will triangulate all objects except the bodies pointed to by ids
%               default value is 0.
%   ids (optional) - the ids to include or exclude in the triangulation
% To triangulate everything, just do orEnvTriangulate(0,[]), or orEnvTriangulate()
%
% Output:
%   tripoints - 3xN matrix of 3D points
%   triindices - 3xK matrix of indices into tripoints for every triangle.
function [tripoints, triindices] = orEnvTriangulate(inclusive, ids)

session = openraveros_getglobalsession();
req = openraveros_env_triangulate();
req.inclusive = inclusive;
if( exist('ids','var') )
    req.bodyids = mat2cell(ids,1,ones(length(ids),1));
end

res = rosoct_session_call(session.id,'env_triangulate',req);

if(~isempty(res))
    numpoints = length(res.points);
    tripoints = reshape(cell2mat(res.points),[3 numpoints/3]);
    
    numinds = length(res.indices);
    triindices = reshape(cell2mat(res.indices),[3 numinds/3]);
else
    tripoints = [];
    triindices = [];
end
