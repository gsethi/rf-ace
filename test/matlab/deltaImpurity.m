function DI = deltaImpurity(x_left,x_right,isNumerical)
%DI = deltaImpurity(x,idx)
%
%Returns decrease in impurity when data x is split into two
%halves, "x_left" and "x_right". Type of data is indicated by 
% isNumerical flag

assert(~any(isnan(x_left)));
assert(~any(isnan(x_right)));

% Calculate the decrease using the variance formula (slow+unstable)
if isNumerical
    
    DI = deltaImpurity_var_regr(x_left,x_right);
    
    %Calculate the decrease using the mean formulat (fast+stable)
    DI_test = deltaImpurity_mean_regr(x_left,x_right);

else
    
    DI = deltaImpurity_gi_class(x_left,x_right);
    
    DI_test = deltaImpurity_sf_class(SF([x_left(:);x_right(:)]),length(x_left) + length(x_right),SF(x_left),length(x_left),SF(x_right),length(x_right));
    
end

%Make sure the two measures agree
if any(isnan([DI,DI_test]))
    assert(isnan(DI) && isnan(DI_test), 'error: only the other impurity function yields NaN');
else
    assert( abs(DI - DI_test ) < 1e-3, 'error: impurity functions disagree in value');
end


function DI = deltaImpurity_mean_regr(x_left,x_right)

x = [x_left(:);x_right(:)];

mu = mean(x);
n = length(x);
muL = mean(x_left);
nL = length(x_left);
muR = mean(x_right);
nR = length(x_right);

DI = -mu^2 + nL/n*muL^2 + nR/n*muR^2;


function DI = deltaImpurity_var_regr(x_left,x_right)

x = [x_left(:);x_right(:)];
n = length(x);
nL = length(x_left);
nR = length(x_right);

DI = var(x,1) - nL/n*var(x_left,1) - nR/n*var(x_right,1);


function DI = deltaImpurity_gi_class(x_left,x_right)

x = [x_left(:);x_right(:)];
n = length(x);
nL = length(x_left);
nR = length(x_right);

DI = giniIndex(x) - nL/n*giniIndex(x_left) - nR/n*giniIndex(x_right);

function DI = deltaImpurity_sf_class(sf_tot,n_tot,sf_left,n_left,sf_right,n_right)

DI = -sf_tot/(n_tot*n_tot) + sf_left/(n_tot*n_left) + sf_right / (n_tot*n_right);

function sf = SF(x)
x = x+1;
sf = sum(hist(x,unique(x)).^2);

function GI = giniIndex(x)

GI = hist(x,unique(x))/length(x);
if ~isempty(GI)
    GI = 1 - sum(GI.^2);
else
    GI = 0;
end

