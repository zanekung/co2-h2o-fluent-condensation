/* Quasi-one-dimensional DRY initial guess, not condensation solution data.
 * Compile this file copied as quasi1d_initialization.c so Fluent's UDF
 * registration scanner sees DEFINE_ON_DEMAND directly. The condensation
 * source files do not need modification. See quasi1d_initialization.md.
 *
 * Frozen x_H2O=0.2 / x_CO2=0.8, ideal gas, variable heat capacity. The first
 * Fluent CO2 cp polynomial (nominally 300--1000 K) is smoothly extended
 * below 300 K ONLY for this initial guess. The solver's material law takes
 * over during iteration. No numerical result is fitted to manuscript plots.
 */
#ifndef CO2H2O_QUASI1D_INITIALIZATION_INCLUDED
#define CO2H2O_QUASI1D_INITIALIZATION_INCLUDED
#include <math.h>
#ifndef Q1D_STANDALONE_TEST
#include "udf.h"
#endif

#define Q1D_PI 3.1415926535897932384626433832795
#define Q1D_RU 8.31446261815324
#define Q1D_MW_W 0.01801528
#define Q1D_MW_C 0.04400950
#define Q1D_P0 500000.0
#define Q1D_T0 390.0
#define Q1D_TREF 298.15
#define Q1D_RT 0.00645
#define Q1D_RIN 0.01995
#define Q1D_RE 0.00775
#define Q1D_XT 0.100
#define Q1D_XE 0.250
#define Q1D_TLOW 150.0

typedef struct {
    double yw, rgas, cp_coeff[5];
    double h0, entropy0, tstar, pstar, rhostar, wstar, gstar, mdot;
} Q1DModel;

typedef struct {
    double temperature, pressure, density, u, v, speed, mach, mach_axial;
    double enthalpy, cp, radius, slope;
} Q1DState;

static double q1d_cp(const Q1DModel *m, double T)
{
    int i;
    double value=m->cp_coeff[4];
    for(i=3;i>=0;--i) value=value*T+m->cp_coeff[i];
    return value;
}

/* Sensible enthalpy in J/kg relative to 298.15 K. */
static double q1d_h(const Q1DModel *m, double T)
{
    int i;
    double h=0.0, ti=T, trefi=Q1D_TREF;
    for(i=0;i<5;++i){
        h+=m->cp_coeff[i]*(ti-trefi)/(i+1);
        ti*=T; trefi*=Q1D_TREF;
    }
    return h;
}

/* Temperature-dependent entropy integral; additive constant cancels. */
static double q1d_entropy(const Q1DModel *m, double T)
{
    int i;
    double result=m->cp_coeff[0]*log(T), ti=T;
    for(i=1;i<5;++i){result+=m->cp_coeff[i]*ti/i;ti*=T;}
    return result;
}

static double q1d_pressure(const Q1DModel *m, double T)
{
    return Q1D_P0*exp((q1d_entropy(m,T)-m->entropy0)/m->rgas);
}

static double q1d_speed_squared(const Q1DModel *m, double T)
{
    double w2=2.0*(m->h0-q1d_h(m,T));
    return w2>0.0?w2:0.0;
}

static double q1d_sound_squared(const Q1DModel *m, double T)
{
    double cp=q1d_cp(m,T);
    return cp/(cp-m->rgas)*m->rgas*T;
}

static double q1d_flux(const Q1DModel *m, double T)
{
    return q1d_pressure(m,T)/(m->rgas*T)*sqrt(q1d_speed_squared(m,T));
}

static void q1d_model_init(Q1DModel *m)
{
    static const double co2_cp[5]={429.92889,1.8744735,-0.001966485,
                                 1.2972514e-6,-3.9999562e-10};
    double lo=Q1D_TLOW,hi=Q1D_T0,mid;
    int i;
    m->yw=0.2*Q1D_MW_W/(0.2*Q1D_MW_W+0.8*Q1D_MW_C);
    m->rgas=Q1D_RU*(m->yw/Q1D_MW_W+(1.0-m->yw)/Q1D_MW_C);
    for(i=0;i<5;++i)m->cp_coeff[i]=(1.0-m->yw)*co2_cp[i];
    m->cp_coeff[0]+=m->yw*1864.0;
    m->h0=q1d_h(m,Q1D_T0);
    m->entropy0=q1d_entropy(m,Q1D_T0);
    for(i=0;i<100;++i){
        mid=0.5*(lo+hi);
        if(q1d_speed_squared(m,mid)>q1d_sound_squared(m,mid))lo=mid;
        else hi=mid;
    }
    m->tstar=0.5*(lo+hi);
    m->pstar=q1d_pressure(m,m->tstar);
    m->rhostar=m->pstar/(m->rgas*m->tstar);
    m->wstar=sqrt(q1d_speed_squared(m,m->tstar));
    m->gstar=m->rhostar*m->wstar;
    m->mdot=Q1D_PI*Q1D_RT*Q1D_RT*m->gstar;
}

static void q1d_geometry(double x, double *radius, double *slope)
{
    if(x<=Q1D_XT){
        double q=x/Q1D_XT;
        double b=1.0-q*q, d=1.0+q*q/3.0;
        double f=b*b/(d*d*d);
        double df=(-4.0*q*b/(d*d*d)-2.0*q*b*b/(d*d*d*d))/Q1D_XT;
        double a=1.0-(Q1D_RT/Q1D_RIN)*(Q1D_RT/Q1D_RIN);
        double denominator=1.0-a*f;
        *radius=Q1D_RT/sqrt(denominator);
        *slope=0.5*Q1D_RT*a*df/pow(denominator,1.5);
    }else{
        *slope=(Q1D_RE-Q1D_RT)/(Q1D_XE-Q1D_XT);
        *radius=Q1D_RT+*slope*(x-Q1D_XT);
    }
}

/* Return 0 on success. -1=outside geometry; -2=target exceeds choked
 * flux; -3=target below supported supersonic branch. No silent clipping.
 * u is axial velocity. v = u*y*R'/R. Solving rho*w with the sqrt factor
 * preserves rho*u*A=mdot while using FULL speed in stagnation enthalpy.
 */
static int q1d_state(const Q1DModel *m,double x,double y,Q1DState *s)
{
    double alpha,factor,target,lo,hi,mid,w2;
    int i;
    if(x<0.0||x>Q1D_XE)return -1;
    q1d_geometry(x,&s->radius,&s->slope);
    if(fabs(y)>s->radius*(1.0+1.e-10))return -1;
    alpha=y*s->slope/s->radius;
    factor=sqrt(1.0+alpha*alpha);
    target=m->gstar*(Q1D_RT/s->radius)*(Q1D_RT/s->radius)*factor;
    if(target>m->gstar*(1.0+1.e-12))return -2;
    if(x<=Q1D_XT){lo=m->tstar;hi=Q1D_T0;}
    else{lo=Q1D_TLOW;hi=m->tstar;}
    if(x>Q1D_XT&&target<q1d_flux(m,lo))return -3;
    if(fabs(target-m->gstar)<=1.e-13*m->gstar){
        lo=m->tstar;hi=m->tstar;
    }else{
        for(i=0;i<100;++i){
            mid=0.5*(lo+hi);
            if(x<=Q1D_XT){
                if(q1d_flux(m,mid)>target)lo=mid;else hi=mid;
            }else{
                if(q1d_flux(m,mid)<target)lo=mid;else hi=mid;
            }
        }
    }
    s->temperature=0.5*(lo+hi);
    s->pressure=q1d_pressure(m,s->temperature);
    s->density=s->pressure/(m->rgas*s->temperature);
    w2=q1d_speed_squared(m,s->temperature);
    s->speed=sqrt(w2);s->u=s->speed/factor;s->v=alpha*s->u;
    s->mach=s->speed/sqrt(q1d_sound_squared(m,s->temperature));
    s->mach_axial=s->u/sqrt(q1d_sound_squared(m,s->temperature));
    s->enthalpy=q1d_h(m,s->temperature);s->cp=q1d_cp(m,s->temperature);
    return 0;
}

#ifndef Q1D_STANDALONE_TEST
DEFINE_ON_DEMAND(co2h2o_quasi1d_initial_guess)
{
#if !RP_HOST
#if RP_3D
    Message0("Quasi1D guess requires this project's 2D axisymmetric mesh. No fields changed.\n");
#else
    Domain *domain=Get_Domain(1);
    Thread *t;
    cell_t c;
    Q1DModel m;
    Q1DState state;
    real xc[ND_ND],p_operating;
    int failed=0,count=0,i;
    if(!Data_Valid_P()||N_UDS<2){
        Message0("Quasi1D guess requires initialized data and two allocated UDS. No fields changed.\n");
        return;
    }
    q1d_model_init(&m);
    /* Validate ALL cells first, including a collective parallel failure check,
     * so a geometry mismatch cannot leave half of the domain patched. */
    thread_loop_c(t,domain){if(FLUID_THREAD_P(t)){
        begin_c_loop(c,t){
            C_CENTROID(xc,c,t);
            if(q1d_state(&m,xc[0],xc[1],&state)!=0)++failed;
        }end_c_loop(c,t)
    }}
#if RP_NODE
    failed=PRF_GISUM1(failed);
#endif
    if(failed){
        Message0("Quasi1D guess rejected: %d incompatible cell locations. No fields changed.\n",failed);
        return;
    }
    p_operating=RP_Get_Real("operating-pressure");
    thread_loop_c(t,domain){if(FLUID_THREAD_P(t)){
        begin_c_loop(c,t){
            double turbulence_k,turbulence_omega;
            C_CENTROID(xc,c,t);
            q1d_state(&m,xc[0],xc[1],&state);
            C_P(c,t)=state.pressure-p_operating;
            C_T(c,t)=state.temperature;
            C_R(c,t)=state.density;
            C_U(c,t)=state.u;C_V(c,t)=state.v;
            if(NNULLP(THREAD_STORAGE(t,SV_H)))C_H(c,t)=state.enthalpy;
            C_YI(c,t,0)=m.yw;C_YI(c,t,1)=1.0-m.yw;
            for(i=0;i<N_UDS;++i)C_UDSI(c,t,i)=0.0;
            for(i=0;i<N_UDM;++i)C_UDMI(c,t,i)=0.0;
            /* A 1% turbulence initial guess; wall/inlet BCs are unchanged. */
            turbulence_k=1.5*0.01*0.01*state.speed*state.speed;
            turbulence_omega=sqrt(turbulence_k)/(pow(0.09,0.25)*0.07*2.0*state.radius);
            if(NNULLP(THREAD_STORAGE(t,SV_K)))C_K(c,t)=turbulence_k;
            if(NNULLP(THREAD_STORAGE(t,SV_O)))C_O(c,t)=turbulence_omega;
            ++count;
        }end_c_loop(c,t)
    }}
#if RP_NODE
    count=PRF_GISUM1(count);
#endif
    Message0("Dry quasi1D INITIAL GUESS patched in %d local/partition cell records.\n",count);
    Message0("p0=%.9g Pa, T0=%.9g K, Y_H2O=%.12g, dry mdot=%.12g kg/s, T*=%.9g K.\n",
             Q1D_P0,Q1D_T0,m.yw,m.mdot,m.tstar);
    Message0("First CO2 cp polynomial extended below 300 K for initialization only.\n");
    Message0("UDS and UDM cleared. This is NOT a converged or condensing result.\n");
    Message0("Use after standard initialization / at steady setup; transient history and flow time are NOT reset here.\n");
#endif
#endif
}
#endif
#endif
