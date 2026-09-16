#include "udf.h"
#include "condensation_core.h"
#include <math.h>

/* Research candidate. H2O must be transported species 0; CO2 is last.
 * 2 UDS, 26 UDM, ideal-gas density, constant H2O heat capacity.
 * Default ICCT coefficient is 1. No source ramp or hidden global multiplier.
 */
#define REQUIRED_UDM 26
#define RU 8.31446261815324
#define MW_W 0.01801528
#define MW_C 0.04400950
enum {SAT,DEW,COOL,J_APPLIED,RCRIT,RADIUS,GROWTH,GAMMA_RAW,GAMMA,
      RHOL,SIGMA,PHI,Z,CLIP,RISK,J_RAW,COND,EVAP,BETA,NVOL,NDEATH,
      JCAP,BIRTH_MASS,GROWTH_MASS,ENERGY_JACOBIAN,ENERGY_CACHED};

static real timestep(void)
{
    real dt=RP_Get_Real("physical-time-step");
    return dt>0?dt:1.e-6;
}

static void evaluate_cell(cell_t c,Thread *t)
{
    Co2H2oCondensationInput in;
    Co2H2oCondensationOutput o;
    real y=C_YI(c,t,0),yl=C_UDSI(c,t,0),nscaled=C_UDSI(c,t,1);
    real velocity_squared=C_U(c,t)*C_U(c,t)+C_V(c,t)*C_V(c,t);
    in.temperature_k=C_T(c,t);
    in.pressure_pa=C_P(c,t)+RP_Get_Real("operating-pressure");
    in.gas_density_kg_m3=C_R(c,t);
    in.h2o_mass_fraction=y;in.liquid_loading_kg_kg=yl;
    in.droplet_number_per_kg=nscaled*CO2H2O_NUMBER_SCALE;
    in.thermal_conductivity_w_m_k=C_K_L(c,t);
    in.dynamic_viscosity_pa_s=C_MU_L(c,t);
    in.cp_j_kg_k=C_CP(c,t);
    y=MAX(0,MIN(1,y));
    in.gas_constant_j_kg_k=RU*(y/MW_W+(1-y)/MW_C);
    in.time_step_s=timestep();
    in.nucleation_correction=1.0;
    in.maximum_nucleation_rate_m3_s=1.e35;
    in.maximum_vapor_consumption_fraction=0.10;
    co2h2o_evaluate(&in,&o);
    C_UDMI(c,t,SAT)=o.supersaturation;
    C_UDMI(c,t,DEW)=o.dewpoint_k;
    C_UDMI(c,t,COOL)=o.supercooling_k;
    C_UDMI(c,t,J_APPLIED)=o.nucleation_rate_m3_s;
    C_UDMI(c,t,RCRIT)=o.critical_radius_m;
    C_UDMI(c,t,RADIUS)=o.droplet_radius_m;
    C_UDMI(c,t,GROWTH)=o.growth_rate_m_s;
    C_UDMI(c,t,GAMMA_RAW)=o.mass_source_raw_kg_m3_s;
    C_UDMI(c,t,GAMMA)=o.mass_source_limited_kg_m3_s;
    C_UDMI(c,t,RHOL)=o.liquid_density_kg_m3;
    C_UDMI(c,t,SIGMA)=o.surface_tension_n_m;
    C_UDMI(c,t,PHI)=1;C_UDMI(c,t,Z)=1;
    C_UDMI(c,t,CLIP)=o.limiter_flag;C_UDMI(c,t,RISK)=o.risk_flag;
    C_UDMI(c,t,J_RAW)=o.nucleation_rate_raw_m3_s;
    C_UDMI(c,t,COND)=o.condensation_applied_kg_m3_s;
    C_UDMI(c,t,EVAP)=o.evaporation_applied_kg_m3_s;
    C_UDMI(c,t,BETA)=MAX(0,yl)/(1+MAX(0,yl));
    C_UDMI(c,t,NVOL)=MAX(0,C_R(c,t))*MAX(0,nscaled)*CO2H2O_NUMBER_SCALE;
    C_UDMI(c,t,NDEATH)=o.number_death_m3_s;
    C_UDMI(c,t,JCAP)=o.rate_cap_flag;
    C_UDMI(c,t,BIRTH_MASS)=o.birth_mass_applied_kg_m3_s;
    C_UDMI(c,t,GROWTH_MASS)=o.growth_mass_applied_kg_m3_s;
    C_UDMI(c,t,ENERGY_CACHED)=co2h2o_energy_source_w_m3(
        o.mass_source_limited_kg_m3_s,in.temperature_k,velocity_squared);
    C_UDMI(c,t,ENERGY_JACOBIAN)=co2h2o_energy_temperature_jacobian(
        &in,velocity_squared,0.01);
}

static void set_field_names(void)
{
    const char *names[REQUIRED_UDM]={
        "h2o-supersaturation","h2o-dewpoint-k","h2o-supercooling-k",
        "nucleation-applied-m3-s","critical-radius-m","droplet-radius-m",
        "droplet-growth-rate-m-s","condensation-source-raw-kg-m3-s",
        "condensation-source-applied-kg-m3-s","liquid-density-kg-m3",
        "surface-tension-n-m","ideal-fugacity-coefficient","ideal-Z",
        "source-limiter-flag","model-risk-flag","nucleation-raw-m3-s",
        "positive-condensation-kg-m3-s","evaporation-kg-m3-s",
        "liquid-mass-fraction-beta","droplet-number-density-m3",
        "empty-droplet-number-sink-m3-s","nucleation-cap-flag",
        "birth-mass-applied-kg-m3-s","growth-mass-applied-kg-m3-s",
        "energy-source-temperature-jacobian-w-m3-k","energy-source-cached-w-m3"};
    int i;
    if(N_UDS>=2){
        Set_User_Scalar_Name(0,"liquid-loading-kg-liquid-per-kg-gas");
        Set_User_Scalar_Name(1,"droplet-number-per-kg-gas-div-1e15");
    }
    if(N_UDM>=REQUIRED_UDM)
        for(i=0;i<REQUIRED_UDM;++i) Set_User_Memory_Name(i,(char *)names[i]);
}

DEFINE_EXECUTE_ON_LOADING(co2h2o_on_loading,library_name)
{
    set_field_names();
    Message0("\n%s: FIG2 RESEARCH CANDIDATE, ideal H2O/CO2, CJ=1.0.\n",library_name);
    Message0("Requires 2 UDS / 26 UDM; UDS1 = N / 1e15.\n");
    Message0("Phase-transfer enthalpy closure only; dilute liquid thermal storage omitted.\n");
}

DEFINE_INIT(co2h2o_init,domain)
{
#if !RP_HOST
    Thread *t;cell_t c;int i;
    if(N_UDS<2 || N_UDM<REQUIRED_UDM) return;
    thread_loop_c(t,domain) if(FLUID_THREAD_P(t)){
        begin_c_loop(c,t){
            C_UDSI(c,t,0)=0;C_UDSI(c,t,1)=0;
            for(i=0;i<REQUIRED_UDM;++i) C_UDMI(c,t,i)=0;
        }end_c_loop(c,t)
    }
#endif
}

static void update_diagnostics(Domain *domain)
{
#if !RP_HOST
    Thread *t;cell_t c;
    if(!Data_Valid_P() || N_UDS<2 || N_UDM<REQUIRED_UDM) return;
    thread_loop_c(t,domain) if(FLUID_THREAD_P(t)){
        begin_c_loop(c,t){evaluate_cell(c,t);}end_c_loop(c,t)
    }
#endif
}

DEFINE_ADJUST(co2h2o_adjust,domain)
{update_diagnostics(domain);}

DEFINE_EXECUTE_AT_END(co2h2o_refresh_at_end)
{
#if !RP_HOST
    update_diagnostics(Get_Domain(1));
#endif
}

DEFINE_ON_DEMAND(co2h2o_refresh_diagnostics)
{
    /* A fresh case read can load this library before UDS/UDM allocation.
     * Restore labels after allocation so resumed exports use the same fields. */
    set_field_names();
#if !RP_HOST
    update_diagnostics(Get_Domain(1));
#endif
    Message0("FIG2 diagnostics refreshed from the current Fluent solution.\n");
}

DEFINE_SOURCE(co2h2o_mass_source,c,t,dS,eqn)
{dS[eqn]=0;return -C_UDMI(c,t,GAMMA);}
DEFINE_SOURCE(co2h2o_h2o_source,c,t,dS,eqn)
{
    dS[eqn]=-C_UDMI(c,t,COND)/MAX(C_YI(c,t,0),1.e-20);
    return -C_UDMI(c,t,GAMMA);
}
DEFINE_SOURCE(co2h2o_liquid_source,c,t,dS,eqn)
{
    dS[eqn]=-C_UDMI(c,t,EVAP)/MAX(C_UDSI(c,t,0),1.e-20);
    return C_UDMI(c,t,GAMMA);
}
DEFINE_SOURCE(co2h2o_number_source,c,t,dS,eqn)
{
    dS[eqn]=-C_UDMI(c,t,NDEATH)
        /(CO2H2O_NUMBER_SCALE*MAX(C_UDSI(c,t,1),1.e-30));
    return (C_UDMI(c,t,J_APPLIED)-C_UDMI(c,t,NDEATH))/CO2H2O_NUMBER_SCALE;
}
DEFINE_SOURCE(co2h2o_xmom_source,c,t,dS,eqn)
{dS[eqn]=-MAX(0,C_UDMI(c,t,GAMMA));return -C_UDMI(c,t,GAMMA)*C_U(c,t);}
DEFINE_SOURCE(co2h2o_ymom_source,c,t,dS,eqn)
{dS[eqn]=-MAX(0,C_UDMI(c,t,GAMMA));return -C_UDMI(c,t,GAMMA)*C_V(c,t);}
DEFINE_SOURCE(co2h2o_energy_source,c,t,dS,eqn)
{
    /* Same ADJUST snapshot as every paired mass/momentum source. The
     * stabilizing Jacobian changes the iteration, not the physical source. */
    dS[eqn]=C_UDMI(c,t,ENERGY_JACOBIAN);
    return C_UDMI(c,t,ENERGY_CACHED);
}
DEFINE_DIFFUSIVITY(co2h2o_uds_diffusivity,c,t,i)
{return C_MU_T(c,t)/0.9+1.e-12;}

DEFINE_SPECIFIC_HEAT(co2h2o_h2o_cp,T,Tref,h,yi)
{*h=CO2H2O_CP_VAPOR*(T-Tref);return CO2H2O_CP_VAPOR;}
DEFINE_SPECIFIC_HEAT(co2h2o_co2_cp,T,Tref,h,yi)
{*h=CO2H2O_CP_CO2*(T-Tref);return CO2H2O_CP_CO2;}
