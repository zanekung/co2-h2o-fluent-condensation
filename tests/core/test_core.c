#include "condensation_core.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846
static int errors=0,checks=0;
static void check(const char *label,int ok){
    ++checks;if(!ok){++errors;printf("FAIL %s\n",label);}
}
static int near(double a,double b,double r){
    return isfinite(a)&&isfinite(b)&&fabs(a-b)<=r*fmax(1.e-30,fmax(fabs(a),fabs(b)));
}
static Co2H2oCondensationInput sample(void){
    Co2H2oCondensationInput x;
    memset(&x,0,sizeof(x));
    x.temperature_k=300;x.pressure_pa=2.e5;x.gas_density_kg_m3=3;
    x.h2o_mass_fraction=0.093;x.thermal_conductivity_w_m_k=.025;
    x.dynamic_viscosity_pa_s=1.5e-5;
    x.cp_j_kg_k=940;x.gas_constant_j_kg_k=215;
    x.time_step_s=1.e-6;x.nucleation_correction=1;
    x.maximum_nucleation_rate_m3_s=1.e35;x.maximum_vapor_consumption_fraction=.1;
    return x;
}

int main(void){
    Co2H2oCondensationInput x;
    Co2H2oCondensationOutput o;
    double t,rcmass,dm,gas0,water0,liquid0,gas1,water1,liquid1,n1;
    double jac1,jac2,jac3,qbefore,qafter;
    int i;
    check("water psat triple point",near(co2h2o_psat_pa(273.16),611.657,.005));
    check("water psat boiling point",near(co2h2o_psat_pa(373.15),101417.99,.0005));
    check("surface tension",near(co2h2o_surface_tension_n_m(300),.07168596,.005));
    check("liquid density",near(co2h2o_liquid_density_kg_m3(373.15),958.35,.002));

    for(i=0;i<8;++i){
        t=240+20*i;
        check("latent = vapor - liquid enthalpy",near(co2h2o_latent_heat_j_kg(t),
            co2h2o_vapor_enthalpy_j_kg(t)-co2h2o_liquid_enthalpy_j_kg(t),1.e-14));
        check("liquid cp is enthalpy derivative",near(
            (co2h2o_liquid_enthalpy_j_kg(t+.01)-co2h2o_liquid_enthalpy_j_kg(t-.01))/.02,
            CO2H2O_CP_LIQUID,1.e-10));
        /* Independent reference-state shift test of the phase-transfer source. */
        check("transfer energy including kinetic energy cancels",fabs(
            co2h2o_energy_source_w_m3(3.0,t,160000)
            +3.0*(co2h2o_liquid_enthalpy_j_kg(t)+80000))<1.e-8);
        x=sample();x.temperature_k=t;x.h2o_mass_fraction=.5;
        x.pressure_pa=co2h2o_psat_pa(t)/co2h2o_h2o_mass_to_mole_fraction(.5);
        check("saturation equilibrium evaluation",co2h2o_evaluate(&x,&o)==0);
        check("ideal saturation equality",near(o.supersaturation,1,1.e-12));
        check("dewpoint inverse matches saturation",fabs(o.dewpoint_k-t)<1.e-6);
        check("equilibrium has no nuclei",o.nucleation_rate_m3_s==0);
    }

    /* Strong source: retain one-to-one relation between applied nuclei and
     * their applied seed mass; old code limited mass but kept raw J. */
    x=sample();x.temperature_k=250;x.maximum_vapor_consumption_fraction=1.e-4;
    check("stiff evaluation",co2h2o_evaluate(&x,&o)==0);
    check("stiff case activates local limiter",o.limiter_flag==1);
    check("limiter also reduces applied J",o.nucleation_rate_m3_s<o.nucleation_rate_raw_m3_s);
    rcmass=4.0/3*PI*pow(o.critical_radius_m,3)*o.liquid_density_kg_m3;
    check("each applied nucleus carries seed mass",near(o.birth_mass_applied_kg_m3_s,
        rcmass*o.nucleation_rate_m3_s,1.e-12));
    check("newly born mass equals net mass without existing drops",near(
        o.mass_source_limited_kg_m3_s,o.birth_mass_applied_kg_m3_s,1.e-12));
    check("available vapor respected",o.condensation_applied_kg_m3_s*x.time_step_s
        <=x.gas_density_kg_m3*x.h2o_mass_fraction*x.maximum_vapor_consumption_fraction*(1+1.e-12));
    gas0=x.gas_density_kg_m3;water0=gas0*x.h2o_mass_fraction;liquid0=0;
    dm=o.mass_source_limited_kg_m3_s*x.time_step_s;
    gas1=gas0-dm;water1=water0-dm;liquid1=liquid0+dm;
    n1=o.nucleation_rate_m3_s*x.time_step_s/gas1;
    check("closed volume total mass",near(gas0+liquid0,gas1+liquid1,1.e-14));
    check("closed volume total water",near(water0+liquid0,water1+liquid1,1.e-14));
    check("closed volume carrier unchanged",near(gas0-water0,gas1-water1,1.e-14));
    check("gas-normalized UDS preserves number",near(gas1*n1,
        o.nucleation_rate_m3_s*x.time_step_s,1.e-14));
    check("new drop radius equals critical radius",near(cbrt(3*(liquid1/gas1)
        /(4*PI*o.liquid_density_kg_m3*n1)),o.critical_radius_m,1.e-12));

    x=sample();x.temperature_k=360;x.h2o_mass_fraction=.001;
    x.liquid_loading_kg_kg=.01;x.droplet_number_per_kg=1.e17;
    check("evaporation evaluation",co2h2o_evaluate(&x,&o)==0);
    check("undersaturated zero nucleation",o.nucleation_rate_m3_s==0);
    check("undersaturated existing drops evaporate",o.mass_source_limited_kg_m3_s<0);
    check("evaporation available liquid respected",o.evaporation_applied_kg_m3_s*x.time_step_s
        <=x.gas_density_kg_m3*x.liquid_loading_kg_kg*x.maximum_vapor_consumption_fraction*(1+1.e-12));
    check("ordinary evaporation keeps droplet number",o.number_source_m3_s==0);
    x.liquid_loading_kg_kg=0;
    co2h2o_evaluate(&x,&o);
    check("empty population decays",o.number_source_m3_s<0);
    check("empty population decay cannot overdraw",-o.number_source_m3_s*x.time_step_s
        <=x.gas_density_kg_m3*x.droplet_number_per_kg*(1+1.e-12));

    /* Dew point inversion away from the current gas temperature. */
    for(i=0;i<8;++i){
        t=245+18*i;x=sample();x.temperature_k=t+23;x.h2o_mass_fraction=.1;
        x.pressure_pa=co2h2o_psat_pa(t)/co2h2o_h2o_mass_to_mole_fraction(.1);
        co2h2o_evaluate(&x,&o);
        check("off-equilibrium dewpoint inversion",fabs(o.dewpoint_k-t)<1.e-5);
    }
    x=sample();x.temperature_k=-1;
    check("invalid temperature rejected",co2h2o_evaluate(&x,&o)!=0 && o.risk_flag==1);

    /* Source Jacobian stability and finite-difference-step sensitivity.
     * The physical core output and caller input must remain unchanged. */
    {
        Co2H2oCondensationInput untouched;
        Co2H2oCondensationOutput before,after;
        x=sample();x.temperature_k=300;x.time_step_s=1.e-20;
        untouched=x;
        co2h2o_evaluate(&x,&before);
        qbefore=co2h2o_energy_source_w_m3(before.mass_source_limited_kg_m3_s,x.temperature_k,160000);
        jac1=co2h2o_energy_temperature_jacobian(&x,160000,.005);
        jac2=co2h2o_energy_temperature_jacobian(&x,160000,.01);
        jac3=co2h2o_energy_temperature_jacobian(&x,160000,.02);
        co2h2o_evaluate(&x,&after);
        qafter=co2h2o_energy_source_w_m3(after.mass_source_limited_kg_m3_s,x.temperature_k,160000);
        check("high S is not inventory limited for Jacobian test",before.limiter_flag==0);
        check("high S energy Jacobian negative",jac1<0 && jac2<0 && jac3<0);
        check("energy Jacobian delta .005 versus .01",near(jac1,jac2,1.e-3));
        check("energy Jacobian delta .01 versus .02",near(jac2,jac3,1.e-3));
        check("Jacobian does not mutate input",memcmp(&x,&untouched,sizeof(x))==0);
        check("Jacobian does not alter physical phase source",memcmp(&before,&after,sizeof(before))==0);
        check("Jacobian leaves physical energy source unchanged",qbefore==qafter);
        check("phase-transfer energy still closes with Jacobian",fabs(qafter+
            after.mass_source_limited_kg_m3_s*(co2h2o_liquid_enthalpy_j_kg(x.temperature_k)+80000))<1.e-8);
        printf("Energy Jacobians at dT=.005/.01/.02 K: %.12e %.12e %.12e W/m3/K\n",jac1,jac2,jac3);
    }
    x=sample();x.temperature_k=250;
    co2h2o_evaluate(&x,&o);
    jac1=co2h2o_energy_temperature_jacobian(&x,160000,.01);
    check("limited source energy derivative includes liquid cp",near(jac1,
        -o.mass_source_limited_kg_m3_s*CO2H2O_CP_LIQUID,1.e-8));
    x=sample();x.temperature_k=300;
    x.pressure_pa=co2h2o_psat_pa(300)/co2h2o_h2o_mass_to_mole_fraction(x.h2o_mass_fraction);
    check("Jacobian crossing saturation switch uses zero",co2h2o_energy_temperature_jacobian(&x,160000,.01)==0);
    check("invalid finite difference step uses zero",co2h2o_energy_temperature_jacobian(&x,160000,-.01)==0);
    x=sample();x.nucleation_correction=0;
    check("no droplets and nucleation disabled gives zero Jacobian",co2h2o_energy_temperature_jacobian(&x,160000,.01)==0);
    printf("%d checks; %d failures. This is implementation verification, not experiment validation.\n",checks,errors);
    return errors?EXIT_FAILURE:EXIT_SUCCESS;
}
