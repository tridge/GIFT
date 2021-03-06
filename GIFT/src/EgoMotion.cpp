/* 
    This file is part of GIFT.

    GIFT is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GIFT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with GIFT.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "EgoMotion.h"
#include <utility>
#include <iostream>

using namespace std;
using namespace Eigen;
using namespace cv;
using namespace GIFT;

EgoMotion::EgoMotion(const vector<pair<Vector3d, Vector3d>>& sphereFlows) {
    Vector3d linVel(0,0,1);
    Vector3d angVel(0,0,0);

    pair<int, double> stepResPair = optimize(sphereFlows, linVel, angVel);
    
    this->optimisedResidual = stepResPair.second;
    this->optimisationSteps = stepResPair.first;
    this->linearVelocity = linVel;
    this->angularVelocity = angVel;
    this->numberOfFeatures = sphereFlows.size();
}

EgoMotion::EgoMotion(const vector<pair<Vector3d, Vector3d>>& sphereFlows, const Vector3d& initLinVel) {
    Vector3d linVel = initLinVel;
    Vector3d angVel = estimateAngularVelocity(sphereFlows, linVel);

    pair<int, double> stepResPair = optimize(sphereFlows, linVel, angVel);
    
    this->optimisedResidual = stepResPair.second;
    this->optimisationSteps = stepResPair.first;
    this->linearVelocity = linVel;
    this->angularVelocity = angVel;
    this->numberOfFeatures = sphereFlows.size();
}

EgoMotion::EgoMotion(const vector<pair<Vector3d, Vector3d>>& sphereFlows, const Vector3d& initLinVel, const Vector3d& initAngVel) {
    Vector3d linVel = initLinVel;
    Vector3d angVel = initAngVel;

    pair<int, double> stepResPair = optimize(sphereFlows, linVel, angVel);
    
    this->optimisedResidual = stepResPair.second;
    this->optimisationSteps = stepResPair.first;
    this->linearVelocity = linVel;
    this->angularVelocity = angVel;
    this->numberOfFeatures = sphereFlows.size();
}

EgoMotion::EgoMotion(const std::vector<Landmark>& landmarks, const double& dt) {
    vector<pair<Vector3d, Vector3d>> sphereFlows;
    for (const auto& lm: landmarks) {
        if (lm.lifetime < 2) continue;
        sphereFlows.emplace_back(make_pair(lm.sphereCoordinates,lm.opticalFlowSphere/dt));
    }

    Vector3d linVel(0,0,1);
    Vector3d angVel(0,0,0);

    pair<int, double> stepResPair = optimize(sphereFlows, linVel, angVel);
    
    this->optimisedResidual = stepResPair.second;
    this->optimisationSteps = stepResPair.first;
    this->linearVelocity = linVel;
    this->angularVelocity = angVel;
    this->numberOfFeatures = sphereFlows.size();

}

EgoMotion::EgoMotion(const vector<GIFT::Landmark>& landmarks, const Vector3d& initLinVel, const double& dt) {
    vector<pair<Vector3d, Vector3d>> sphereFlows;
    for (const auto& lm: landmarks) {
        if (lm.lifetime < 2) continue;
        sphereFlows.emplace_back(make_pair(lm.sphereCoordinates,lm.opticalFlowSphere/dt));
    }

    Vector3d linVel = initLinVel;
    Vector3d angVel = estimateAngularVelocity(sphereFlows, linVel);

    pair<int, double> stepResPair = optimize(sphereFlows, linVel, angVel);
    
    this->optimisedResidual = stepResPair.second;
    this->optimisationSteps = stepResPair.first;
    this->linearVelocity = linVel;
    this->angularVelocity = angVel;
    this->numberOfFeatures = sphereFlows.size();
}

EgoMotion::EgoMotion(const std::vector<Landmark>& landmarks, const Vector3d& initLinVel, const Vector3d& initAngVel, const double& dt) {
    vector<pair<Vector3d, Vector3d>> sphereFlows;
    for (const auto& lm: landmarks) {
        if (lm.lifetime < 2) continue;
        sphereFlows.emplace_back(make_pair(lm.sphereCoordinates,lm.opticalFlowSphere/dt));
    }
    Vector3d linVel = initLinVel;
    Vector3d angVel = initAngVel;

    pair<int, double> stepResPair = optimize(sphereFlows, linVel, angVel);
    
    this->optimisedResidual = stepResPair.second;
    this->optimisationSteps = stepResPair.first;
    this->linearVelocity = linVel;
    this->angularVelocity = angVel;
    this->numberOfFeatures = sphereFlows.size();
}

pair<int,double> EgoMotion::optimize(const vector<pair<Vector3d, Vector3d>>& flows, Vector3d& linVel, Vector3d& angVel) {
    double lastResidual = 1e8;
    double residual = computeResidual(flows, linVel, angVel);

    Vector3d bestLinVel = linVel;
    Vector3d bestAngVel = angVel;
    double bestResidual = INFINITY;
    int optimisationSteps = 0;

    int iteration = 0;
    while ((abs(lastResidual - residual) > optimisationThreshold) && (iteration < maxIterations)) {
        lastResidual = residual;
        optimizationStep(flows, linVel, angVel);
        residual = computeResidual(flows, linVel, angVel);
        ++iteration;

        // cout << "residual: " << residual << endl;

        if (residual < bestResidual) {
            bestResidual = residual;
            bestLinVel = linVel;
            bestAngVel = angVel;
            optimisationSteps = iteration;
        }
    }

    linVel = bestLinVel;
    angVel = bestAngVel;

    if (voteForLinVelInversion(flows, linVel, angVel)) {
        linVel = -linVel;
    }

    return make_pair(optimisationSteps, bestResidual);
}

double EgoMotion::computeResidual(const vector<pair<Vector3d, Vector3d>>& flows, const Vector3d& linVel, const Vector3d& angVel) {
    Vector3d wHat = linVel.normalized();

    double residual = 0;
    int normalisationFactor = 0;
    for (const auto& flow : flows) {
        const Vector3d& phi = flow.second;
        const Vector3d& eta = flow.first;

        double res_i = wHat.dot((phi + angVel.cross(eta)).cross(eta));
        residual += pow(res_i,2);
        ++normalisationFactor;
    }
    normalisationFactor = max(normalisationFactor,1);
    residual = residual / normalisationFactor;

    return residual;
}

void EgoMotion::optimizationStep(const std::vector<pair<Vector3d, Vector3d>>& flows, Vector3d& linVel, Vector3d& angVel) {
    auto Proj3 = [](const Vector3d& vec) { return Matrix3d::Identity() - vec*vec.transpose()/vec.squaredNorm(); };

    Vector3d wHat = linVel.normalized();

    Matrix3d tempHess11 = Matrix3d::Zero();
    Matrix3d tempHess12 = Matrix3d::Zero();
    Matrix3d tempHess22 = Matrix3d::Zero();
    Vector3d tempGrad2 = Vector3d::Zero();

    for (const auto& flow : flows) {
        // Each flow is a pair of spherical bearing eta and perpendicular flow vector phi.
        const Vector3d& phi = flow.second;
        const Vector3d& eta = flow.first;

        Vector3d ZOmega = (phi + angVel.cross(eta)).cross(eta);
        Matrix3d ProjEta = Proj3(eta);

        tempHess11 += ZOmega*ZOmega.transpose();
        tempHess12 += wHat.transpose()*ZOmega*ProjEta + ZOmega*wHat.transpose()*ProjEta;
        tempHess22 += ProjEta*wHat*wHat.transpose()*ProjEta;

        tempGrad2 += wHat.transpose()*ZOmega*ProjEta*wHat;
    }

    Matrix<double, 6,6> hessian;
    Matrix<double, 6,1> gradient;

    Matrix3d ProjWHat = Proj3(wHat);
    hessian.block<3,3>(0,0) = ProjWHat*tempHess11*ProjWHat;
    hessian.block<3,3>(0,3) = -ProjWHat*tempHess12;
    hessian.block<3,3>(3,0) = hessian.block<3,3>(0,3).transpose();
    hessian.block<3,3>(3,3) = tempHess22;

    gradient.block<3,1>(0,0) = ProjWHat*tempHess11*wHat;
    gradient.block<3,1>(3,0) = -tempGrad2;

    // Step with Newton's method
    // Compute the solution to Hess^{-1} * grad
    Matrix<double,6,1> step = hessian.bdcSvd(ComputeFullU | ComputeFullV).solve(gradient);
    wHat += -step.block<3,1>(0,0);

    linVel = linVel.norm() * wHat.normalized();
    angVel += -step.block<3,1>(3,0);
}

vector<pair<Point2f, Vector2d>> EgoMotion::estimateFlowsNorm(const vector<GIFT::Landmark>& landmarks) const {
    vector<pair<Vector3d, Vector3d>> flowsSphere = estimateFlows(landmarks);
    vector<pair<Point2f, Vector2d>> flowsNorm;

    for (const auto& flowSphere: flowsSphere) {
        const Vector3d& eta = flowSphere.first;
        const Vector3d& phi = flowSphere.second;

        double eta3 = eta.z();
        Point2f etaNorm(eta.x() / eta3, eta.y() / eta3);
        Vector2d phiNorm;
        phiNorm << 1/eta3 * (phi.x() - etaNorm.x*phi.z()),
                   1/eta3 * (phi.y() - etaNorm.y*phi.z());
        flowsNorm.emplace_back(make_pair(etaNorm, phiNorm));
    }

    return flowsNorm;
}

vector<pair<Vector3d, Vector3d>> EgoMotion::estimateFlows(const vector<GIFT::Landmark>& landmarks) const {
    auto Proj3 = [](const Vector3d& vec) { return Matrix3d::Identity() - vec*vec.transpose()/vec.squaredNorm(); };

    vector<pair<Vector3d, Vector3d>> estFlows;

    for (const auto& lm: landmarks) {
        const Vector3d& eta = lm.sphereCoordinates;
        const Vector3d etaVel = Proj3(eta) * this->linearVelocity;

        double invDepth = 0;
        if (etaVel.norm() > 0) invDepth = -etaVel.dot(lm.opticalFlowSphere + this->angularVelocity.cross(eta)) / etaVel.squaredNorm();

        Vector3d flow = -this->angularVelocity.cross(eta) + invDepth * etaVel;
        estFlows.emplace_back(make_pair(eta, flow));
    }

    return estFlows;
}

Vector3d EgoMotion::estimateAngularVelocity(const vector<pair<Vector3d, Vector3d>>& sphereFlows, const Vector3d& linVel) {
    // Uses ordinary least squares to estimate angular velocity from linear velocity and flows.
    auto Proj3 = [](const Vector3d& vec) { return Matrix3d::Identity() - vec*vec.transpose()/vec.squaredNorm(); };
    auto skew = [](const Vector3d& v) {
    Matrix3d m;
    m << 0, -v(2), v(1),
         v(2), 0, -v(0),
         -v(1), v(0), 0;
    return m;
    };

    bool velocityFlag = (linVel.norm() > 1e-4);

    Matrix3d tempA = Matrix3d::Zero();
    Vector3d tempB = Vector3d::Zero();

    for (const auto & flow : sphereFlows) {
        const Vector3d& eta = flow.first;
        const Vector3d& phi = flow.second;

        tempA += Proj3(eta);
        if (velocityFlag) tempB += eta.cross(Proj3( Proj3(eta)*linVel )*phi);
        else tempB += eta.cross(phi);
    }

    Vector3d angVel = -tempA.inverse()*tempB;
    return angVel;
}

bool EgoMotion::voteForLinVelInversion(const vector<pair<Vector3d, Vector3d>>& flows, const Vector3d& linVel, const Vector3d& angVel) {
    auto Proj3 = [](const Vector3d& vec) { return Matrix3d::Identity() - vec*vec.transpose()/vec.squaredNorm(); };
    int invertVotes = 0;

    for (const auto& flowPair: flows) {
        const Vector3d& eta = flowPair.first;
        const Vector3d etaVel = Proj3(eta) * linVel;

        double scaledInvDepth = -etaVel.dot(flowPair.second + angVel.cross(eta));
        if (scaledInvDepth > 0) --invertVotes;
        else if (scaledInvDepth < 0) ++invertVotes;
    }

    bool invertDecision = (invertVotes > 0);
    return invertDecision;
}