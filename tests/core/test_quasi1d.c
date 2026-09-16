#define Q1D_STANDALONE_TEST
#include "quasi1d_initialization.inc"
#include <stdio.h>

static int failures=0,checks=0;
static void check(int condition,const char *name)
{++checks;if(!condition){++failures;printf("FAIL %s\n",name);}}
static double max(double a,double b){return a>b?a:b;}

int main(void)
{
    Q1DModel m;
    Q1DState s,sp,sm;
    double xs[401],eta[76],r,dr,a,x,y,relative_mdot;
    double max_h=0,max_p0=0,max_mass=0,max_mach_branch=0;
    double max_geometry_slope=0,max_continuity=0,max_dcp=0;
    int segments[3]={70,180,150},k=0,i,j,seg,ret,valid=0;
    double endpoints[4]={0.0,0.07,0.13,0.25};
    q1d_model_init(&m);
    check(fabs(m.yw-0.09283677142689885)<1.e-14,"mass fraction");
    check(fabs(m.rgas-214.23143731848387)<1.e-10,"gas constant");
    check(fabs(m.wstar*m.wstar/q1d_sound_squared(&m,m.tstar)-1)<1.e-13,"critical Mach=1");
    for(i=0;i<=400;++i){
        double T=200.0+190.0*i/400.0,delta=0.001;
        double numerical_cp=(q1d_h(&m,T+delta)-q1d_h(&m,T-delta))/(2*delta);
        max_dcp=max(max_dcp,fabs(numerical_cp/q1d_cp(&m,T)-1));
    }
    check(max_dcp<1.e-9,"enthalpy derivative equals cp");
    xs[0]=0;
    for(seg=0;seg<3;++seg)
        for(i=1;i<=segments[seg];++i)
            xs[++k]=endpoints[seg]+(endpoints[seg+1]-endpoints[seg])*i/segments[seg];
    for(j=0;j<=75;++j)eta[j]=tanh(3.0*j/75.0)/tanh(3.0);
    for(i=0;i<400;++i){
        /* Matching the mean of four quad vertices used by the initial
         * centroid audit. Fluent's exact cell centroids are checked by the
         * UDF before patching; the axial center stays inside this interval. */
        double ra,rb,dummy;
        x=0.5*(xs[i]+xs[i+1]);
        q1d_geometry(xs[i],&ra,&dummy);q1d_geometry(xs[i+1],&rb,&dummy);
        q1d_geometry(x,&r,&dr);
        for(j=0;j<75;++j){
            y=0.25*(ra+rb)*(eta[j]+eta[j+1]);
            ret=q1d_state(&m,x,y,&s);
            check(ret==0,"all actual mesh row midpoints have a valid branch");
            if(ret)continue;
            ++valid;
            max_h=max(max_h,fabs((s.enthalpy+0.5*(s.u*s.u+s.v*s.v)-m.h0)/m.h0));
            a=s.pressure*exp((m.entropy0-q1d_entropy(&m,s.temperature))/m.rgas);
            max_p0=max(max_p0,fabs(a/Q1D_P0-1));
            relative_mdot=s.density*s.u*Q1D_PI*r*r/m.mdot;
            max_mass=max(max_mass,fabs(relative_mdot-1));
            max_mach_branch=max(max_mach_branch,x<Q1D_XT?max(0,s.mach-1):max(0,1-s.mach));
        }
        if(x<0.0999||x>0.1001){
            double delta=1.e-7,rp,rm,unused,flux_derivative,radial_divergence;
            q1d_geometry(x+delta,&rp,&unused);q1d_geometry(x-delta,&rm,&unused);
            max_geometry_slope=max(max_geometry_slope,fabs((rp-rm)/(2*delta)-dr));
            /* Fixed physical y when differentiating the axial flux. */
            y=0.4*r;
            q1d_state(&m,x+delta,y,&sp);q1d_state(&m,x-delta,y,&sm);
            q1d_state(&m,x,y,&s);
            flux_derivative=(sp.density*sp.u-sm.density*sm.u)/(2*delta);
            radial_divergence=2*s.density*s.u*dr/r;
            max_continuity=max(max_continuity,fabs(flux_derivative+radial_divergence)/(m.gstar/Q1D_XT));
        }
    }
    check(max_h<1.e-12,"total enthalpy invariant");
    check(max_p0<1.e-12,"total pressure invariant");
    check(max_mass<1.e-10,"axial mass flow invariant");
    check(max_mach_branch==0,"correct subsonic and supersonic branches");
    check(max_geometry_slope<1.e-8,"analytic Witoszynski derivative");
    check(max_continuity<1.e-7,"axisymmetric continuity differential check");
    check(q1d_state(&m,-0.1,0,&s)==-1,"out-of-domain rejected");
    check(q1d_state(&m,0.1,0.1,&s)==-1,"outside wall rejected");
    /* The throat slope has a genuine geometric kink. The tiny off-axis
     * interval immediately after it cannot support this exact construction;
     * it must be rejected rather than hide a flux limit. No mesh centroid
     * lies in this interval. */
    q1d_geometry(0.1+1.e-9,&r,&dr);
    check(q1d_state(&m,0.1+1.e-9,0.999*r,&s)==-2,"geometric kink incompatibility rejected");
    printf("cp_derivative_rel_error=%.12g h0_rel_error=%.12g p0_rel_error=%.12g mdot_rel_error=%.12g\n",
           max_dcp,max_h,max_p0,max_mass);
    printf("geometry_slope_abs_error=%.12g continuity_scaled_error=%.12g\n",max_geometry_slope,max_continuity);
    printf("Tstar_K=%.12g Pstar_Pa=%.12g wstar_m_s=%.12g dry_mdot_kg_s=%.12g\n",m.tstar,m.pstar,m.wstar,m.mdot);
    for(i=0;i<3;++i){
        x=i==0?0.0:(i==1?Q1D_XT:Q1D_XE);
        ret=q1d_state(&m,x,0,&s);
        check(ret==0,"axis station valid");
        printf("axis x=%.8g T=%.12g p=%.12g u=%.12g M=%.12g A_At=%.12g\n",
               x,s.temperature,s.pressure,s.u,s.mach,(s.radius/Q1D_RT)*(s.radius/Q1D_RT));
    }
    printf("checks=%d failures=%d mesh_points=%d\n",checks,failures,valid);
    return failures?1:0;
}
