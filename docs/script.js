/**
 * AuraUI Documentation Site Scripts
 * Interactive Code Copying, ScrollSpy, Search Filter & Mobile Drawer
 */

document.addEventListener('DOMContentLoaded', () => {
    // 1. Code Copy Functionality
    const codeContainers = document.querySelectorAll('.code-container');
    codeContainers.forEach((container) => {
        const copyBtn = container.querySelector('.btn-copy');
        const codeElement = container.querySelector('pre code');
        
        if (copyBtn && codeElement) {
            copyBtn.addEventListener('click', async () => {
                const text = codeElement.innerText;
                try {
                    await navigator.clipboard.writeText(text);
                    const originalText = copyBtn.innerHTML;
                    copyBtn.innerHTML = `<span>✓</span> Copied`;
                    copyBtn.style.color = 'var(--text-primary)';
                    copyBtn.style.borderColor = 'var(--border-strong)';
                    
                    setTimeout(() => {
                        copyBtn.innerHTML = originalText;
                        copyBtn.style.color = '';
                        copyBtn.style.borderColor = '';
                    }, 2000);
                } catch (err) {
                    console.error('Failed to copy code:', err);
                }
            });
        }
    });

    // 2. ScrollSpy for Active Sidebar Navigation
    const navLinks = document.querySelectorAll('.nav-link');
    const sections = Array.from(navLinks).map(link => {
        const id = link.getAttribute('href');
        if (id && id.startsWith('#')) {
            return document.querySelector(id);
        }
        return null;
    }).filter(Boolean);

    function updateActiveLink() {
        const scrollPosition = window.scrollY + 120;
        let currentSection = null;

        for (const section of sections) {
            if (section.offsetTop <= scrollPosition) {
                currentSection = section;
            }
        }

        navLinks.forEach(link => link.classList.remove('active'));

        if (currentSection) {
            const activeLink = document.querySelector(`.nav-link[href="#${currentSection.id}"]`);
            if (activeLink) {
                activeLink.classList.add('active');
            }
        } else if (navLinks.length > 0) {
            navLinks[0].classList.add('active');
        }
    }

    window.addEventListener('scroll', updateActiveLink, { passive: true });
    updateActiveLink();

    // 3. Search Filter
    const searchInput = document.getElementById('search-docs');
    if (searchInput) {
        searchInput.addEventListener('input', (e) => {
            const query = e.target.value.toLowerCase().trim();
            if (!query) {
                navLinks.forEach(l => l.parentElement.style.display = '');
                return;
            }

            navLinks.forEach(link => {
                const text = link.textContent.toLowerCase();
                if (text.includes(query)) {
                    link.parentElement.style.display = '';
                } else {
                    link.parentElement.style.display = 'none';
                }
            });
        });
    }

    // 4. Mobile Menu Drawer Toggle
    const mobileMenuBtn = document.querySelector('.mobile-menu-btn');
    const sidebar = document.querySelector('.docs-sidebar');

    if (mobileMenuBtn && sidebar) {
        mobileMenuBtn.addEventListener('click', () => {
            sidebar.classList.toggle('open');
            mobileMenuBtn.innerHTML = sidebar.classList.contains('open') ? '✕' : '☰';
        });

        // Close when clicking a nav link on mobile
        navLinks.forEach(link => {
            link.addEventListener('click', () => {
                if (window.innerWidth <= 820) {
                    sidebar.classList.remove('open');
                    mobileMenuBtn.innerHTML = '☰';
                }
            });
        });
    }

    // 5. Back to Top Button
    const backToTopBtn = document.getElementById('back-to-top');
    if (backToTopBtn) {
        window.addEventListener('scroll', () => {
            if (window.scrollY > 400) {
                backToTopBtn.classList.add('visible');
            } else {
                backToTopBtn.classList.remove('visible');
            }
        }, { passive: true });

        backToTopBtn.addEventListener('click', () => {
            window.scrollTo({ top: 0, behavior: 'smooth' });
        });
    }

    // 6. Interactive Demo Widget (Sum calculator in docs)
    const demoCalcBtn = document.getElementById('demo-calc-run');
    if (demoCalcBtn) {
        demoCalcBtn.addEventListener('click', () => {
            const valA = parseFloat(document.getElementById('demo-num-a').value) || 0;
            const valB = parseFloat(document.getElementById('demo-num-b').value) || 0;
            const resultDisplay = document.getElementById('demo-calc-res');
            resultDisplay.textContent = `Result: ${valA + valB}`;
            resultDisplay.style.color = 'var(--text-primary)';
        });
    }
});
