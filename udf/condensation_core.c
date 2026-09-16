#include "condensation_core.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

#define PI 3.141592653589793238462643383279502884
#define RU 8.31446261815324
#define KB 1.380649e-23
#define NA 6.02214076e23
#define MW_W 0.01801528
#define MW_C 0.04400950
#define TC 647.096
#define PC 22.064e6

static double bound(double x, double lo, double hi)
{ return x < lo ? lo : (x > hi ? hi : x); }

double co2h2o_psat_pa(double t)
{
    static const double a[6] = {-7.85951783,1.84408259,-11.7866497,
        22.6807411,-15.9618719,1.80122502};
    static const double b[6] = {1.0,1.5,3.0,3.5,4.0,7.5};
    double tau, sum = 0.0;
    int i;
    t = bound(t,123.0,TC);
    if(t <= 273.16) return exp(54.842763-6763.22/t-4.210*log(t)
        +0.000367*t+tanh(0.0415*(t-218.8))
        *(53.878-1331.22/t-9.44523*log(t)+0.014025*t));
    tau=1.0-t/TC;
    for(i=0;i<6;++i) sum+=a[i]*pow(tau,b[i]);
    return PC*exp(TC/t*sum);
}

double co2h2o_liquid_density_kg_m3(double t)
{
    static const double a[6]={1.99274064,1.09965342,-0.510839303,
        -1.75493479,-45.5170352,-6.74694450e5};
    static const double b[6]={1.0/3,2.0/3,5.0/3,16.0/3,43.0/3,110.0/3};
    double tau=1.0-bound(t,200.0,TC)/TC, sum=1.0;
    int i;
    for(i=0;i<6;++i) sum+=a[i]*pow(tau,b[i]);
    return fmax(1.0,322.0*sum);
}

double co2h2o_surface_tension_n_m(double t)
{
    double tau=fmax(0.0,1.0-bound(t,200.0,TC)/TC);
    return 0.2358*pow(tau,1.256)*(1.0-0.625*tau);
}

double co2h2o_vapor_enthalpy_j_kg(double t)
{ return CO2H2O_CP_VAPOR*(t-CO2H2O_TREF); }
double co2h2o_liquid_enthalpy_j_kg(double t)
{ return CO2H2O_CP_LIQUID*(t-CO2H2O_TREF)-CO2H2O_LREF; }
double co2h2o_latent_heat_j_kg(double t)
{ return co2h2o_vapor_enthalpy_j_kg(t)-co2h2o_liquid_enthalpy_j_kg(t); }
double co2h2o_energy_source_w_m3(double gamma,double t,double u2)
{
    /* New condensate mass carries liquid enthalpy and gas-matched velocity.
     * This closes the PHASE-TRANSFER contribution. Existing liquid thermal
     * storage and acceleration are neglected by the dilute gas+UDS model. */
    return -gamma*(co2h2o_liquid_enthalpy_j_kg(t)+0.5*u2);
}
double co2h2o_h2o_mass_to_mole_fraction(double y)
{
    double w=bound(y,0.0,1.0)/MW_W;
    double c=(1.0-bound(y,0.0,1.0))/MW_C;
    return w/(w+c);
}

static double dewpoint(double pw,double t_initial)
{
    /* Safeguarded Newton solve of ln(p_sat(T)) = ln(pw), with the
     * Clausius-Clapeyron slope used only as a numerical derivative estimate. */
    double lo=123.0,hi=TC,t=bound(t_initial,lo,hi), target, err, next;
    int i;
    if(pw<=co2h2o_psat_pa(lo)) return lo;
    if(pw>=PC) return hi;
    target=log(pw);
    for(i=0;i<24;++i){
        err=log(co2h2o_psat_pa(t))-target;
        if(fabs(err)<1.e-10) break;
        if(err>0) hi=t; else lo=t;
        next=t-err*(RU/MW_W)*t*t/co2h2o_latent_heat_j_kg(t);
        t=(next>lo && next<hi)?next:0.5*(lo+hi);
    }
    return t;
}

int co2h2o_evaluate(const Co2H2oCondensationInput *in,
                   Co2H2oCondensationOutput *o)
{
    double t,p,rho,y,yl,n,pw,rhol,sigma,s,td,rc=0,r=0,J=0,G=0;
    double birth,growth,pos,neg,pos_scale=1.0,neg_scale=1.0,fraction,dt;
    double mass_one=0.0,latent;
    if(!in || !o) return 1;
    memset(o,0,sizeof(*o));
    t=in->temperature_k;p=in->pressure_pa;rho=in->gas_density_kg_m3;
    if(!isfinite(t)||!isfinite(p)||!isfinite(rho)||t<=0||p<=0||rho<=0
       ||!isfinite(in->h2o_mass_fraction)||!isfinite(in->liquid_loading_kg_kg)
       ||!isfinite(in->droplet_number_per_kg)) {o->risk_flag=1;return 2;}
    y=bound(in->h2o_mass_fraction,0,1);
    yl=fmax(0,in->liquid_loading_kg_kg);n=fmax(0,in->droplet_number_per_kg);
    if(t<248 || t>600 || yl>0.1 || in->h2o_mass_fraction<0
       ||in->liquid_loading_kg_kg<0||in->droplet_number_per_kg<0) o->risk_flag=1;
    rhol=co2h2o_liquid_density_kg_m3(t);sigma=co2h2o_surface_tension_n_m(t);
    pw=co2h2o_h2o_mass_to_mole_fraction(y)*p;
    s=pw/co2h2o_psat_pa(t);td=dewpoint(pw,t);
    latent=co2h2o_latent_heat_j_kg(t);
    if(t>=TC) s=0;
    if(s>1+1.e-12 && sigma>0 && y>0){
        double lns=log(s),m=MW_W/NA,v=m/rhol;
        double a=pow(36*PI,1.0/3)*pow(v,2.0/3);
        double theta=sigma*a/(KB*t);
        double barrier=16*PI*pow(sigma,3)*m*m
            /(3*pow(KB,3)*pow(t,3)*rhol*rhol*lns*lns);
        double cap=fmax(1.0,in->maximum_nucleation_rate_m3_s);
        double logj;
        rc=2*sigma/(rhol*(RU/MW_W)*t*lns);
        /* ICCT water form: exp(theta)/S correction. C_J=1 is the
         * uncalibrated research default; it is not tuned to any figure. */
        if(in->nucleation_correction>0){
            logj=log(in->nucleation_correction)-lns+2*log(rho*y)-log(rhol)
                +0.5*log(2*sigma/(PI*m*m*m))-barrier+theta;
            if(logj>log(cap)){J=cap;o->rate_cap_flag=1;}
            else if(logj>-700) J=exp(logj);
        }
    }
    if(yl>1.e-30 && n>1.e-30) r=cbrt(3*yl/(4*PI*rhol*n));
    else r=fmax(rc,1.e-9);
    r=bound(r,1.e-10,1.e-3);
    if(yl>1.e-30 && n>1.e-30 && t<TC && latent>0
       &&in->thermal_conductivity_w_m_k>0 &&in->dynamic_viscosity_pa_s>0
       &&in->gas_constant_j_kg_k>0 &&in->cp_j_kg_k>in->gas_constant_j_kg_k){
        double k=in->thermal_conductivity_w_m_k,mu=in->dynamic_viscosity_pa_s;
        double cp=in->cp_j_kg_k,R=in->gas_constant_j_kg_k;
        double Pr=mu*cp/k,gamma=cp/(cp-R);
        double mfp=mu/p*sqrt(PI*R*t/2),Kn=mfp/(2*r);
        double correction=1+2*sqrt(8*PI)/(1.5*Pr)*gamma/(gamma+1)*Kn;
        double curvature=rc>0?1-rc/r:1;
        G=k*curvature*(td-t)/(rhol*latent*r*correction);
    }
    mass_one=4.0/3*PI*rc*rc*rc*rhol;
    birth=mass_one*J;
    growth=4*PI*r*r*rhol*rho*n*G;
    pos=birth+fmax(0,growth);neg=fmax(0,-growth);
    o->mass_source_raw_kg_m3_s=pos-neg;
    dt=in->time_step_s;
    if(!isfinite(dt)||dt<=0){o->risk_flag=1;dt=1.e-6;}
    fraction=bound(in->maximum_vapor_consumption_fraction,1.e-6,1);
    if(pos>0) pos_scale=fmin(1,rho*y*fraction/(dt*pos));
    if(neg>0) neg_scale=fmin(1,rho*yl*fraction/(dt*neg));
    o->condensation_applied_kg_m3_s=pos*pos_scale;
    o->evaporation_applied_kg_m3_s=neg*neg_scale;
    o->mass_source_limited_kg_m3_s=pos*pos_scale-neg*neg_scale;
    o->nucleation_rate_raw_m3_s=J;
    o->nucleation_rate_m3_s=J*pos_scale;
    o->birth_mass_applied_kg_m3_s=birth*pos_scale;
    o->growth_mass_applied_kg_m3_s=growth>=0?growth*pos_scale:growth*neg_scale;
    /* A monodisperse population keeps its count during ordinary evaporation.
     * Remove numerically empty populations, avoiding an unphysical permanent
     * number inventory after liquid has vanished. This cutoff is diagnostic. */
    if(yl<=1.e-16 && n>0 && s<=1) o->number_death_m3_s=rho*n*fraction/dt;
    o->number_source_m3_s=o->nucleation_rate_m3_s-o->number_death_m3_s;
    o->supersaturation=s;o->dewpoint_k=td;o->supercooling_k=td-t;
    o->critical_radius_m=rc;o->droplet_radius_m=(yl>1.e-30 && n>1.e-30)?r:0;
    o->growth_rate_m_s=G;o->liquid_density_kg_m3=rhol;o->surface_tension_n_m=sigma;
    o->limiter_flag=pos_scale<1-1.e-12 || neg_scale<1-1.e-12;
    return 0;
}

double co2h2o_energy_temperature_jacobian(
    const Co2H2oCondensationInput *in,double u2,double dt)
{
    Co2H2oCondensationInput plus,minus;
    Co2H2oCondensationOutput op,om;
    double qp,qm,derivative;
    if(!in || !isfinite(dt) || dt<=0 || in->temperature_k<=dt) return 0;
    plus=*in;minus=*in;
    plus.temperature_k+=dt;minus.temperature_k-=dt;
    /* This is a partial derivative with density, pressure, mass fractions,
     * moments and transport properties frozen at the ADJUST snapshot. */
    if(co2h2o_evaluate(&plus,&op)!=0 || co2h2o_evaluate(&minus,&om)!=0) return 0;
    /* Do not linearize across a major constitutive/limiter switch. Returning
     * zero changes only the Jacobian; the central physical source is retained. */
    if((op.supersaturation>1)!=(om.supersaturation>1)
       ||op.limiter_flag!=om.limiter_flag||op.rate_cap_flag!=om.rate_cap_flag) return 0;
    qp=co2h2o_energy_source_w_m3(op.mass_source_limited_kg_m3_s,plus.temperature_k,u2);
    qm=co2h2o_energy_source_w_m3(om.mass_source_limited_kg_m3_s,minus.temperature_k,u2);
    derivative=(qp-qm)/(2*dt);
    if(!isfinite(derivative)) return 0;
    return fmin(0,derivative);
}
